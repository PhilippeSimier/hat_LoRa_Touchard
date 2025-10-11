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

int main(int argc, char** argv) {

    try {

        SSD1306 oled;

        oled.begin();
        oled.setTextSize(2);

        oled << clear << argv[1] << "\n" << argv[2] << display;

    }


    catch (const std::runtime_error &e) {

        cout << "Exception caught: " << e.what() << endl;
    }

    return 0;
}

