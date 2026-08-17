/**
    @file security.hpp
    @brief Classe du chiffrage légé
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#pragma once

#include <Arduino.h>

class security
{
private:
    static const String private_key;      ///< Clée privé pour chiffé la communication
    static const char b64_chars[];        ///< Liste des charactères en base64

public:
    security(/* args */);
    ~security();
    static String let_unxor_msg(const String& encoded_msg);
    static String let_xor_msg(const String& msg);
    static String base64_decode(const String& input);
    static String base64_encode(const String& input);
};

