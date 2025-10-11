/* 
 * File:   main.cpp
 * Author: philippe SIMIER Lycée Touchard
 *
 * Created on 5 octobre 2024, 08:54
 */

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "SSD1306.h"

using namespace std;

void afficherTexte(SSD1306& oled, const string& ligne1, const string& ligne2) {
    oled.begin();
    oled.setTextSize(2);
    oled << clear << ligne1 << "\n" << ligne2 << display;
}

int main(int argc, char** argv) {

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <ligne1> <ligne2>" << endl;
        return EXIT_FAILURE;
    }

    try {

        SSD1306 oled;
        afficherTexte(oled, argv[1], argv[2]);
        sleep(1);
    }
    catch (const std::runtime_error &e) {

        cout << "Exception caught: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        cerr << "Erreur inconnue" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

