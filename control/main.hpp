#pragma once

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <queue>
#include <algorithm>
#include <cstdlib>
#include <ctime>


///   Structures   ///
struct messages { // Permet la gestion des messages via une fil d'attente
    std::string sender_ip;
    std::string data;
};


///   Fonction   ///
int send_msg_on_network(const char* ip_drone, std::string msg);
void find_my_drone();
void thread_reception();
void manage_answer();
int generate_paquet_id();
void delete_paquet_id(int paquet_id);
std::string base64_encode(const std::string& input);
std::string base64_decode(const std::string& input);
std::string let_xor_msg(const std::string& msg);
std::string let_unxor_msg(const std::string& encoded_msg);
std::string trim(const std::string& str);


///   Variables   ///
std::string _ip_drone;
bool drone_trouve = false;
std::string private_key = "3f906ab3b876a65d912195e8fea28f5a2117caa65317516adf7e71d5e1be40f6";      ///< Clée privé pour chiffé la communication
const char b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";  ///< Liste des charactères en base64
const char* __msg = nullptr;
constexpr int OUT_PORT = 14550;
constexpr int IN_PORT = 14551;
constexpr int MINIMAL_DELAY_OF_LOOP = 20 + 5;

const char* taokoff_msg = "TAKEOFF_5M";
std::string derniere_telemetrie = "En attente de données...";
std::mutex mutex_telemetrie;
std::atomic<bool> application_en_cours(true); // Booléen spécial conçu pour être lu/écrit par plusieurs threads sans risque
std::queue<messages> Messages; 
std::mutex mutex_messages;

/// Liste des commandes ///
std::string discovery_msg = let_xor_msg("WHO_IS_DRONE_OF_KING");
std::string CMD_LEFT = "LEFT";
std::string CMD_RIGHT  = "RIGHT";
std::string CMD_UP = "UP";
std::string CMD_DOWN = "DOWN";
std::string CMD_DO_A_FLIP = "DO_A_FLIP";
std::string CMD_TAKEOFF = "TAKEOFF";
std::string CMD_LAND = "LAND";
std::string CMD_GET_BATTERIE = "GET_BATTERY";
std::string CMD_GET_ALTITUDE = "GET_ALTITUDE";
std::string CMD_GET_YAW_AND_PITCH = "GET_YAW_AND_PITCH";
std::string CMD_GET_POSITION = "GET_POSITION";
std::string CMD_EMERGENCY_STOP = "EMERGENCY_STOP";
std::string CMD_WHO_IS_DRONE_OF_KING = "WHO_IS_DRONE_OF_KING";
std::string FORWARD = "FORWARD";
std::string BACKWARD = "BACKWARD";

std::vector<int> waiting_awsers_paquet_id; // Liste des IDs de paquet attendus

