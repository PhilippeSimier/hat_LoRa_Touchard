/* 
 * File:   main.cpp
 * Author: philippe SIMIER (F4JRE)
 *
 * Programme APRS-IS Client
 * 
 *  Created on 15 septembre 2025, 11:29
 */

#include <iostream>
#include <thread>
#include <csignal>
#include "AprsClient.h"
#include "Position.h"
#include "GestionFile.h"

using namespace std;

static bool stopRequested = false;

GestionFile fileRX;
MessageRX message;

// Gestionnaire du signal SIGINT (Ctrl+C)
void signalHandler(int signal);

// Fonction callback appelée à chaque ligne reçue du serveur APRS-IS
void onAprsMessageReceived(const std::string& message);

// Fonction pour retransmettre les trames reçues de Lora vers le serveur APRS-IS
void retransmitLoRaToAprsIs(AprsClient& aprs,
                            const std::string& loraFrame,
                            const std::string& igateCallsign);



int main() {
    // enregistre le handler pour   SIGINT (Ctrl+C)
    std::signal(SIGINT, signalHandler);

    try {

        fileRX.obtenirFileIPC(5678);

        // --- 1️Création du client APRS-IS ---
        AprsClient aprs;

        // --- 2️ Connexion au serveur APRS-IS ---
        aprs.connectToServer("euro.aprs2.net", 14580);

        // --- 3️ Authentification avec un filtre---
        aprs.authenticate("F4JRE-3", "r/48.01013/0.20614/100");

        // --- 4️ Création d'une position pour apparaître sur la carte APRS ---
        Position pos(48.01013, 0.20614, 85, 'I', '&', "C++ Client"); // latitude, longitude, altitude, symbole, commentaire

        // --- 5️ Affichage du locator sur la console ---
        cout << "Locator : " << pos.getLocator(6) << endl;

        // --- 6️ Envoi d'un beacon initial ---
        aprs.sendPosition(pos);
        cout << "[APRS] Beacon initial envoyé." << endl;

        // --- 7️ Démarrage du thread d’écoute APRS-IS ---
        aprs.startListening(onAprsMessageReceived);

        // --- 8️ Boucle principale ---
        while (!stopRequested) {

            if (stopRequested) break;

            message = fileRX.lireDansLaFileIPC(2);
            string loraFrame(message.text);
            retransmitLoRaToAprsIs(aprs, loraFrame, "F4JRE-3");


            this_thread::sleep_for(chrono::seconds(1));
        }

        // --- 10 Arrêt propre ---
        cout << "[APRS] Arrêt du thread d’écoute et fermeture du socket..." << endl;
        aprs.stopListening();
        aprs.disconnect();
        cout << "[APRS] Déconnexion terminée. Fin du programme." << endl;
    } catch (const runtime_error& e) {
        cerr << "Erreur fatale APRS : " << e.what() << endl;
        return EXIT_FAILURE;
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\n[APRS] Arrêt demandé (SIGINT reçu)..." << endl;
        stopRequested = true;
    }
}

void onAprsMessageReceived(const std::string& message) {
    
    if (!message.empty() && message[0] == '#') {
        return; // ignore toutes les lignes de commentaires serveur
    }

    std::cout << "[APRS RX] " << message;
}

void retransmitLoRaToAprsIs(AprsClient& aprs,
                            const std::string& loraFrame,
                            const std::string& igateCallsign)
{
    if (loraFrame.empty() || loraFrame[0] == '#')
        return;

    size_t pos_gt = loraFrame.find('>');
    size_t pos_colon = loraFrame.find(':');

    if (pos_gt == std::string::npos || pos_colon == std::string::npos) {
        std::cerr << "[LoRa] Trame invalide ignorée : " << loraFrame << std::endl;
        return;
    }

    // Extraire la partie avant ":" pour insérer le tag
    std::string head = loraFrame.substr(0, pos_colon);
    std::string data = loraFrame.substr(pos_colon); // incluant le ':'

    // Vérifie si un tag qAR est déjà présent
    if (head.find("qAR") != std::string::npos) {
        std::cout << "[LoRa] Trame déjà marquée iGate, ignorée : " << loraFrame << std::endl;
        return;
    }

    // Ajout du tag iGate ",qAR,<callsign>"
    std::string frameWithTag = head + ",qAR," + igateCallsign + data;

    try {
        aprs.sendLine(frameWithTag);
        std::cout << "[LoRa→APRS-IS] " << frameWithTag << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "[LoRa→APRS-IS] Erreur d’envoi : " << e.what() << std::endl;
    }
}



