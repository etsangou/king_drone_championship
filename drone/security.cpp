/**
    @file security.cpp
    @brief Classe du chiffrage légé
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#include "security.hpp"

const String security::private_key = "3f906ab3b876a65d912195e8fea28f5a2117caa65317516adf7e71d5e1be40f6";
const char security::b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

security::security() {

}

security::~security() = default;

String security::base64_encode(const String& input) {
    String output;
    int val = 0;
    int valb = -6;

    for (size_t i = 0; i < input.length(); i++) {
        unsigned char c = input[i];
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output += b64_chars[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }

    if (valb > -6) {
        output += b64_chars[((val << 8) >> (valb + 8)) & 0x3F];
    }

    while (output.length() % 4) {
        output += '=';
    }

    return output;
}

String security::base64_decode(const String& input) {
    int T[256];
    for (int i = 0; i < 256; i++) T[i] = -1;
    for (int i = 0; i < 64; i++) T[(unsigned char)b64_chars[i]] = i;

    String output;
    int val = 0;
    int valb = -8;

    for (size_t i = 0; i < input.length(); i++) {
        unsigned char c = input[i];
        if (c == '=') break;
        if (T[c] == -1) continue;

        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            output += char((val >> valb) & 0xFF);
            valb -= 8;
        }
    }

    return output;
}

String security::let_xor_msg(const String& msg) {
    String xored;
    xored.reserve(msg.length());

    for (size_t i = 0; i < msg.length(); i++) {
        unsigned char a = (unsigned char)private_key[i % private_key.length()];
        unsigned char b = (unsigned char)msg[i];
        xored += char(a ^ b);
    }

    return base64_encode(xored);
}

String security::let_unxor_msg(const String& encoded_msg) {
    String xored = base64_decode(encoded_msg);
    String msg;
    msg.reserve(xored.length());

    for (size_t i = 0; i < xored.length(); i++) {
        unsigned char a = (unsigned char)private_key[i % private_key.length()];
        unsigned char b = (unsigned char)xored[i];
        msg += char(a ^ b);
    }

    return msg;
}
