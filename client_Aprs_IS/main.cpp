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
#include "SimpleIni.h"

#define CONFIGURATION "/opt/configuration.ini"

using namespace std;

static bool stopRequested = false;

GestionFile fileRX;
MessageRX message;

// Gestionnaire du signal SIGINT (Ctrl+C)
void signalHandler(int signal);

// Fonction callback appelée à chaque ligne reçue du serveur APRS-IS
void onAprsMessageReceived(const std::string& message);

int main() {
    // enregistre le handler pour   SIGINT (Ctrl+C)
    std::signal(SIGINT, signalHandler);
    SimpleIni ini;


    try {

        ini.Load(CONFIGURATION);
        string indicatif = ini.GetValue("aprs", "indicatif", "F4ABC");
        double latitude = ini.GetValue<double>("beacon", "latitude", 48.0);
        double longitude = ini.GetValue<double>("beacon", "longitude", 0.0);
        double altitude = ini.GetValue<double>("beacon", "altitude", 1);
        string comment = ini.GetValue<string>("beacon", "comment", "comment default");
        char symbol_table = ini.GetValue<char>("beacon", "symbol_table", '/');
        char symbol  = ini.GetValue<char>("beacon", "symbol", 'O');

        fileRX.obtenirFileIPC(5678);

        // --- 1️ Création du client APRS-IS bidirectionnel---
        AprsClient aprs(true);

        // --- 2️ Connexion au serveur APRS-IS ---
        aprs.connectToServer("euro.aprs2.net", 14580);

        // --- 3️ Authentification avec un filtre---
        aprs.authenticate( indicatif, "r/48.01013/0.20614/20");

        // --- 4️ Création d'une balise sur la carte APRS ---
        Position pos(latitude, longitude, altitude, symbol_table, symbol, comment); // latitude, longitude, altitude, symbole, commentaire

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
            aprs.retransmitFrame(loraFrame);

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





