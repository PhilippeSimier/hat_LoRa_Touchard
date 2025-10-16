/* 
 * File:   AprsClient.h
 * Author: philippe SIMIER (F4JRE)
 *
 * Created on 15 septembre 2025, 11:30
 * 
 * connectToServer()	Connexion TCP robuste avec message de statut
 * authenticate()	Login automatique APRS-IS avec calcul de pass
 * sendPosition()	Envoie direct d’une trame APRS 
 * receiveAsync()	Lecture non bloquante via select()
 * startListening()	Thread d’écoute indépendant
 * reconnect()          Reconnexion automatique + ré-authentification
 * stopListening() 	Arrêt propre du thread et du socket
 * disconnect()         Deconnexion du serveur
 * 
 */



#ifndef APRSCLIENT_H
#define APRSCLIENT_H

#include <string>
#include <thread>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "Position.h"

class AprsClient {
    
public:
    AprsClient();
    ~AprsClient();

    void connectToServer(const std::string& host, int port);
    void authenticate(const std::string& callsign, const std::string& filter);
    void sendLine(const std::string& line);
    void sendPosition(Position& pos);
    void disconnect();

    void startListening(std::function<void(const std::string&)> onMessage);
    void stopListening();

private:
    int sockfd;
    std::atomic<bool> connected;
    std::atomic<bool> running;
    std::thread listenerThread;
    std::function<void(const std::string&)> messageCallback;

    std::string serverHost;
    int serverPort;
    std::string callsign;
    std::string filterOption;

    std::string receiveAsync(int timeoutSec);
    void setNonBlocking(bool enable);
    bool reconnect();
    int computePasscode(const std::string& callsign);
};

#endif // APRSCLIENT_H

