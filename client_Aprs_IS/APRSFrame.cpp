/* 
 * File:   APRSFrame.cpp
 * Author: philippe SIMIER (F4JRE)
 * 
 * Created on 1 août 2025, 08:10
 */

#include "APRSFrame.h"

APRSFrame::APRSFrame(const std::string& frame) :
rawFrame(frame),
type(FrameType::Unknown) {
    parse();
}

APRSFrame::APRSFrame() :
type(FrameType::Unknown) {

}

void APRSFrame::setRaw(const std::string& frame) {
    rawFrame = frame;
    type = FrameType::Unknown;
    source.clear();
    destination.clear();
    path.clear();
    message.clear();
    addressee.clear();
    hasPosition = false;
    parse();
}

APRSFrame::APRSFrame(const APRSFrame& orig) {
}

APRSFrame::~APRSFrame() {
}

void APRSFrame::parse() {

    size_t gt_pos = rawFrame.find('>');
    size_t colon_pos = rawFrame.find(':');

    if (gt_pos == std::string::npos || colon_pos == std::string::npos)
        return;

    source = rawFrame.substr(0, gt_pos);

    std::string dest_and_path = rawFrame.substr(gt_pos + 1, colon_pos - gt_pos - 1);
    size_t comma_pos = dest_and_path.find(',');

    if (comma_pos != std::string::npos) {
        destination = dest_and_path.substr(0, comma_pos);
        path = dest_and_path.substr(comma_pos + 1);
    } else {
        destination = dest_and_path;
    }
    // fin du décodage entête APRS

    std::string payload = rawFrame.substr(colon_pos + 1);

    if (!payload.empty()) {
        char c = payload[0];
        if (c == '!' || c == '=' || c == '/' || c == '@') {
            type = FrameType::Position;
            if (isCompressed(payload)) {
                parseCompressedPosition(payload);
            } else {  
                parseUncompressedPosition(payload);
            }

        } else if (c == '>') {
            type = FrameType::Status;

        } else if (c == '_') {
            type = FrameType::Weather;

        } else if (payload.rfind("T#", 0) == 0) { // commence par "T#"
            type = FrameType::Telemetry;

        } else if (c == ':' && payload[10] == ':') {
            
            type = FrameType::Message;
            addressee = payload.substr(1, 9);
            rtrim(addressee);
            message = payload.substr(11);
            rtrim(message);

        } else {
            type = FrameType::Unknown;
        }
    }

    

}

void APRSFrame::print() const {
    std::cout << "Raw : " << rawFrame << "\n";
    std::cout << "Source        : " << source << "\n";
    std::cout << "Destination   : " << destination << "\n";
    std::cout << "Path          : " << path << "\n";
    std::cout << "Type de trame : " << typeToString(type) << "\n";

    if (type == FrameType::Message) {
        std::cout << "Addressee     : " << addressee << "\n";
        std::cout << "Message       : " << message << "\n";
    }

    if (hasPosition) {
        std::cout << "Symbole APRS  : " << symbolTable << symbolCode << " → " << getSymbolDescription() << std::endl;
        std::cout << std::setprecision(5);
        std::cout << "Latitude      : " << latitude << "\n";
        std::cout << "Longitude     : " << longitude << "\n";

        if (hasAltitude) {
            std::cout << "Altitude      : " << altitudeFeet << " ft (" << std::fixed << std::setprecision(1) << altitudeMetre << " m)\n";
        }
    }

    std::cout << "\n";
}

std::string APRSFrame::typeToString(FrameType type) {

    switch (type) {
        case APRSFrame::FrameType::Message: return "Message";
        case APRSFrame::FrameType::Position: return "Position";
        case APRSFrame::FrameType::Status: return "Status";
        case APRSFrame::FrameType::Telemetry: return "Telemetry";
        case APRSFrame::FrameType::Weather: return "Weather";
        default: return "Unknown";
    }
}

std::string APRSFrame::getSource() const {
    return source;
}

std::string APRSFrame::getDestination() const {
    return destination;
}

std::string APRSFrame::getPath() const {
    return path;
}

std::string APRSFrame::getAddressee() const {
    return addressee;
}

std::string APRSFrame::getMessage() const {
    return message;
}

APRSFrame::FrameType APRSFrame::getFrameType() const {
    return type;
}

double APRSFrame::getLatitude() const {
    return latitude;
}

double APRSFrame::getLongitude() const {
    return longitude;
}

double APRSFrame::getAltitude() const {
    return altitudeMetre;
}

void APRSFrame::parseUncompressedPosition(std::string payload) {

    hasPosition = false;
    hasAltitude = false;

    if (payload.length() < 19) return; // trop court

    std::string lat_str = payload.substr(1, 8); // 8 caractères : ddmm.mmN
    std::string lon_str = payload.substr(10, 9); // 9 caractères : dddmm.mmE

    try {
        latitude = parseCoordinate(lat_str, lat_str.back());
        longitude = parseCoordinate(lon_str, lon_str.back());
        hasPosition = true;
        symbolTable = payload[9]; // SYMBOL overlay
        symbolCode = payload[19];

        // ALTITUDE: chercher "/A="
        size_t alt_pos = payload.find("/A=");
        if (alt_pos != std::string::npos && alt_pos + 9 < payload.size()) {
            std::string alt_str = payload.substr(alt_pos + 3, 6);
            altitudeFeet = std::stoi(alt_str);
            altitudeMetre = altitudeFeet * 0.3048; // conversion en m
            hasAltitude = true;
        }

    } catch (...) {
        hasPosition = false;
        hasAltitude = false;
    }

}

/**
 * Vérifie si une trame position est compressée ou pas
 * Le caractère qui suit le ! n'est pas un chiffre c'est le symboleTable
 * @param payload
 * @return true ou false
 */
bool APRSFrame::isCompressed(const std::string& payload) {

    return (payload.size() >= 13 && !std::isdigit(static_cast<unsigned char> (payload[1])));


}

void APRSFrame::parseCompressedPosition(std::string payload) {

    hasPosition = false;
    hasAltitude = false;

    symbolTable = payload[1];
    std::string lat_str = payload.substr(2, 4);
    std::string lon_str = payload.substr(6, 4);
    symbolCode = payload[10];

    long lat_val = base91ToDecimal(lat_str);
    long lon_val = base91ToDecimal(lon_str);

    latitude = 90.0 - (lat_val / 380926.0);
    longitude = -180.0 + (lon_val / 190463.0);
    hasPosition = true;

}

double APRSFrame::parseCoordinate(const std::string& coord, char direction) {
    // Ex: 4803.50N or 00145.12E
    double deg = std::stod(coord.substr(0, coord.find('.') - 2));
    double min = std::stod(coord.substr(coord.find('.') - 2));
    double decimal = deg + (min / 60.0);

    if (direction == 'S' || direction == 'W') {
        decimal = -decimal;
    }

    return decimal;
}

std::string APRSFrame::getSymbolDescription() const {
    static const std::map<std::string, std::string> symbolMap = {
        {"/>", "Car"},
        {"/<", "Motorcycle"},
        {"/b", "Bicycle"},
        {"/O", "Balloon"},
        {"/_", "Weather station"},
        {"/-", "House"},
        {"/*", "Snowmobile"},
        {"/[", "Human"},
        {"/k", "Truck"},
        {"/r", "Repeater tower"},
        {"/s", "Ship, power boat"},
        {"/Y", "Sailboat"},
        {"/v", "Van"},
        {"\\O", "Rocket"},
        {"\\-", "House, HF antenna"},
        {"\\^", "Aircraft"},
        {"\\s", "Ship, boat"},
        {"\\S", "Satellite"},
        {"L#", "Digipeater, green star + L"},
        {"La", "Red diamond + L"},
        {"L_", "Weather site + L"},
        {"L&", "Gateway station + L"},
        {"R&", "Gateway station + R"},
        {"D&", "Gateway station + D"},
    };

    std::string key;
    key.push_back(symbolTable); // ex: '/' ou '\'
    key.push_back(symbolCode); // ex: 'O', '>', etc.

    auto it = symbolMap.find(key);
    if (it != symbolMap.end())
        return it->second;
    else
        return "Symbole inconnu (" + key + ")";
}

// Fonction utilitaire pour supprimer les espaces en fin de chaîne

void APRSFrame::rtrim(std::string &s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char> (s.back()))) {
        s.pop_back();
    }
}

/**
 * Décodeur Base91
 * @param str
 * @return  la valeur decimal décodée
 */
long APRSFrame::base91ToDecimal(const std::string& str) {

    long value = 0;
    for (char c : str) {
        value = value * 91 + (c - 33);
    }
    return value;
}
