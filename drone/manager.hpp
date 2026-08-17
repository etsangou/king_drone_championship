/**
    @file manager.hpp
    @brief Classe de gestion du drone
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <queue>
#include "security.hpp"
#include "drive.hpp"

// Liste des commandes que le drone peu recevoir
namespace commands {
    const String LEFT = "LEFT";
    const String RIGHT = "RIGHT";
    const String UP = "UP";
    const String DOWN = "DOWN";
    const String DO_A_FLIP = "DO_A_FLIP";
    const String TAKEOFF = "TAKEOFF";
    const String LAND = "LAND";
    const String GET_BATTERY = "GET_BATTERY";
    const String GET_ALTITUDE = "GET_ALTITUDE";
    const String GET_YAW_AND_PITCH = "GET_YAW_AND_PITCH";
    const String GET_POSITION = "GET_POSITION";
    const String EMERGENCY_STOP = "EMERGENCY_STOP";
    const String WHO_IS_DRONE_OF_KING = "WHO_IS_DRONE_OF_KING";
    const String FORWARD = "FORWARD";
    const String BACKWARD = "BACKWARD";

    const String ALL_COMMANDS[] = {
        LEFT, RIGHT, UP, DOWN, DO_A_FLIP, TAKEOFF, LAND, 
        GET_BATTERY, GET_ALTITUDE, GET_YAW_AND_PITCH, GET_POSITION
    };
    
    const int NUM_COMMANDS = sizeof(ALL_COMMANDS) / sizeof(ALL_COMMANDS[0]);

    // Verifie l'existance de la commande
    inline bool isValid(const String& cmd) {
        for (int i = 0; i < NUM_COMMANDS; i++) {
            if (cmd == ALL_COMMANDS[i]) {
                return true;
            }
        }
        return false;
    }
}

struct paquet { // Permet la gestion des messages via une fil d'attente
    String payload;
    String paquetID;
};

// Classe
class manager {
private:
    bool _isOK = false, _personalComputerIPKnowed = false, _appStart = false;
    char _packetBuffer[255]; // Tableau pour stocker le message entrant
    const String DEFAULT_OUT_CMD = "INVALIDE_CMD";

    const int IN_COMMUNICATION = 14550;
    const int OUT_COMMUNICATION = 14551;

    // Object atribut
    drive* _flyManager = nullptr;;
    WiFiUDP _udp;
    IPAddress _personalComputerIP;
    std::queue<paquet> _messagesQueue; 
    TaskHandle_t _multithread = NULL;

    // Méthodes
    void setPersonalComputerIP(IPAddress newIP);
    void sendData(String msg);
    static void answerMannager(void * pvParameters);
    void addCommandToQueue(String command, String paquetID);

public:
    manager();
    ~manager();

    void initProtocol();
    void initWirelessConnexion();
    void waitingPCCall(); // On attend jusqu'à ce que le PC envoie : "WHO_IS_DRONE_OF_KING"
    void startListening();
    void start();
    void stop();
};
