/* 
 * File:   AprsClient.cpp
 * Author: philippe SIMIER (F4JRE)
 * 
 * Created on 15 septembre 2025, 11:30
 */

#include "AprsClient.h"

AprsClient::AprsClient(bool _bidirectionnel) :
    sockfd(-1),
    bidirectionnel(_bidirectionnel),
    connected(false),
    running(false)
{
}

AprsClient::~AprsClient() {
    stopListening();
    disconnect();
}

void AprsClient::connectToServer(const std::string& host, int port) {
    struct hostent* server = gethostbyname(host.c_str());
    if (!server)
        throw std::runtime_error("Serveur introuvable : " + host);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw std::runtime_error("Erreur socket()");

    struct sockaddr_in serv_addr {
    };
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof (serv_addr)) < 0)
        throw std::runtime_error("Impossible de se connecter à " + host);

    connected = true;
    serverHost = host;
    serverPort = port;

    std::cout << "[APRS] Connecté à " << host << ":" << port << std::endl;
}

void AprsClient::authenticate(const std::string& call, const std::string& filter) {
    if (!connected)
        throw std::runtime_error("Non connecté au serveur APRS-IS");

    int pass = computePasscode(call);

    std::string login = "user " + call +
            " pass " + std::to_string(pass) +
            " vers C++ APRSClient filter " + filter + "\n";

    send(sockfd, login.c_str(), login.size(), 0);

    callsign = call;
    filterOption = filter;

    std::cout << "[APRS] Authentifié en tant que " << call << " (pass=" << pass << ")" << std::endl;
}

void AprsClient::sendLine(const std::string& line) {
    if (!connected)
        throw std::runtime_error("Non connecté au serveur APRS-IS");

    std::string msg = line;
    if (msg.back() != '\n')
        msg += "\n";

    ssize_t n = send(sockfd, msg.c_str(), msg.size(), 0);
    if (n < 0)
        throw std::runtime_error("Erreur lors de l'envoi : " + line);
}

void AprsClient::sendPosition(Position& pos) {
    std::string headAprs = callsign + ">APRS:";
    sendLine(headAprs + pos.getPduAprs());
}

void AprsClient::disconnect() {
    if (connected) {
        close(sockfd);
        connected = false;
        std::cout << "[APRS] Déconnecté du serveur." << std::endl;
    }
}

void AprsClient::stopListening() {
    if (running) {
        running = false;
        if (listenerThread.joinable()) listenerThread.join();
    }
}

void AprsClient::setNonBlocking(bool enable) {
    if (sockfd < 0) throw std::runtime_error("Socket invalide");

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) throw std::runtime_error("Erreur fcntl(F_GETFL)");

    if (enable)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (fcntl(sockfd, F_SETFL, flags) < 0)
        throw std::runtime_error("Erreur fcntl(F_SETFL)");
}

std::string AprsClient::receiveAsync(int timeoutSec) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    struct timeval tv {
        timeoutSec, 0
    };

    int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);

    if (ret < 0) {
        connected = false;
        return "";
    }
    if (ret == 0) return "";

    if (FD_ISSET(sockfd, &readfds)) {
        char buffer[1024] = {0};
        ssize_t n = recv(sockfd, buffer, sizeof (buffer) - 1, 0);

        if (n > 0) {
            return std::string(buffer, n);
        } else if (n == 0) {
            connected = false;
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
            connected = false;
        }
    }

    return "";
}

bool AprsClient::reconnect() {
    try {
        disconnect();
        connectToServer(serverHost, serverPort);
        authenticate(callsign, filterOption);
        return true;
    } catch (const std::runtime_error& e) {
        std::cerr << "[APRS] Erreur de reconnexion : " << e.what() << std::endl;
        return false;
    }
}

void AprsClient::startListening(std::function<void(const std::string&) > onMessage) {
    
    if (!connected)
        throw std::runtime_error("Non connecté au serveur APRS-IS");

    messageCallback = onMessage;
    running = true;

    listenerThread = std::thread([this]() {
        std::cout << "[APRS] Thread d'écoute démarré." << std::endl;

        while (running) {
            try {
                std::string data = receiveAsync(1);

                if (!data.empty()) {
                    if (data.find("disconnected") != std::string::npos ||
                            data.find("lost") != std::string::npos) {
                        connected = false;
                    }

                    if (connected && messageCallback)
                            messageCallback(data);
                    }

                if (!connected) {
                    std::cerr << "[APRS] Tentative de reconnexion..." << std::endl;

                            bool ok = reconnect();
                    if (ok) {
                        std::cout << "[APRS] Reconnecté avec succès." << std::endl;
                                connected = true;
                    } else {
                        std::cerr << "[APRS] Échec de la reconnexion. Nouvel essai dans 10 s." << std::endl;
                                std::this_thread::sleep_for(std::chrono::seconds(10));
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));

            } catch (const std::runtime_error& e) {
                std::cerr << "[APRS] Erreur d'écoute : " << e.what() << std::endl;
                        connected = false;
                        std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }

        std::cout << "[APRS] Thread d'écoute arrêté." << std::endl;
    });
}

int AprsClient::computePasscode(const std::string& callsign) {

    std::string call;

    // Ne garder que la partie avant le "-"
    size_t pos = callsign.find('-');
    if (pos != std::string::npos) {
        call = callsign.substr(0, pos);
    } else {
        call = callsign;
    }

    // En majuscules
    for (auto& c : call) {
        c = std::toupper(static_cast<unsigned char> (c));
    }

    int hash = 0x73E2;
    for (size_t i = 0; i < call.size(); i++) {
        hash ^= (call[i] << ((i & 1) ? 0 : 8));
    }

    return hash & 0x7FFF; // borne sur 15 bits 
}

// Fonction pour retransmettre les trames vers le serveur APRS-IS
void AprsClient::retransmitFrame(const std::string& loraFrame) {

    if (!connected)
        throw std::runtime_error("Non connecté au serveur APRS-IS");

    if (loraFrame.empty() || loraFrame[0] == '#')
        throw std::runtime_error("retransmitFrame vide");

    size_t pos_gt = loraFrame.find('>');
    size_t pos_colon = loraFrame.find(':');

    if (pos_gt == std::string::npos || pos_colon == std::string::npos)
        throw std::runtime_error("Trame invalide ignorée");


    // Extraire la partie avant ":" pour insérer le tag
    std::string head = loraFrame.substr(0, pos_colon);
    std::string data = loraFrame.substr(pos_colon); // incluant le ':'

    // Vérifie si la trame vient déjà d’un iGate (contient qAR, qAO, etc.)
    if (loraFrame.find("qAR") != std::string::npos ||
        loraFrame.find("qAO") != std::string::npos ||
        loraFrame.find("qAS") != std::string::npos)
        throw std::runtime_error("Trame déjà marquée iGate, ignorée");


    // Sélection du tag iGate
    std::string tag = bidirectionnel ? "qAO" : "qAR";

    std::string frameWithTag = head + ',' + tag  + ',' + callsign + data;
    sendLine(frameWithTag);
    std::cout << "[LoRa→APRS-IS] " << frameWithTag << std::endl;

}







