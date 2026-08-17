#include "main.hpp"

int main() {
    std::cout << "[APP] Démarrage de l'application..." << std::endl;

    // Random help
    srand(time(0));

    // On lance la fonction 'thread_reception' en arrière-plan
    std::thread background_worker(thread_reception);
    std::thread background_treatments(manage_answer);

    // On cherche le drone sur le réseau
    std::cout << "[APP] Recherche du drone en cours (Broadcast)..." << std::endl;
    for (int i = 0; i < 3; i++) {
        find_my_drone();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 3 secondes
        if(drone_trouve) break;
        else std::cout << "[APP] Essai " << i+1 << " echoué" << std::endl;
    }

    // On lance la communication et l'acces au commande
    if (drone_trouve) {
        std::cout << "[APP] Lancement du protocol de communication avec le drone {" << _ip_drone << "}" << std::endl;
        send_msg_on_network(_ip_drone.c_str(), CMD_EMERGENCY_STOP);
        send_msg_on_network(_ip_drone.c_str(), CMD_LEFT);
        send_msg_on_network(_ip_drone.c_str(), CMD_GET_BATTERIE);
        send_msg_on_network(_ip_drone.c_str(), CMD_GET_YAW_AND_PITCH);


        std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // 3 secondes
    }

    // On ferme tout
    std::cout << "[APP] Fermeture du programme..." << std::endl;
    // On attend un peu que les derniers accusés de réception arrivent
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Affichage de tout les paquets perdus
    for (auto i : waiting_awsers_paquet_id) std::cout << i << std::endl;

    application_en_cours = false; // Dit au thread de s'arrêter
    background_worker.join();     // On attend que le thread ait bien fini avant de quitter
    background_treatments.join(); // On supprime l'objet pour éviter les fuites de mémoire
    return 0;
}

void find_my_drone() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    // 2. Activer l'autorisation d'envoyer en Broadcast
    int broadcastEnable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    // 3. Configurer un timeout de réception (ex: 2 secondes) pour ne pas bloquer
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 4. Configurer l'adresse de destination en Broadcast
    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(OUT_PORT);
    // 255.255.255.255 signifie "envoyer à tout le monde sur le réseau local"
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); 

    // 5. Envoyer le message de découverte
    sendto(sock, discovery_msg.c_str(), discovery_msg.length(), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
}

int send_msg_on_network(const char* ip_drone, std::string msg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    sockaddr_in drone_addr;
    drone_addr.sin_family = AF_INET;
    drone_addr.sin_port = htons(OUT_PORT); // Port pour la télémétrie
    inet_pton(AF_INET, ip_drone, &drone_addr.sin_addr); // IP du drone

    // Construction du payload à envoyer 
    std::string random_id = std::to_string(generate_paquet_id());
    std::string command = msg ;
    command += "\n" + random_id;
    // std::cout << "Commande" << command << std::endl;
    command = let_xor_msg(command) + "\r\n";

    // Envoi du message
    ssize_t sent_bytes = sendto(sock, command.c_str(), command.length(), 0, (struct sockaddr*)&drone_addr, sizeof(drone_addr));

    close(sock);
    return 0;
}

// --- LE THREAD DE RÉCEPTION  ---
void thread_reception() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    // 1. IMPORTANT : Mettre un timeout sur le socket (ex: 1 seconde)
    // Sinon recvfrom bloque pour toujours et on ne pourra jamais fermer le thread proprement
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(IN_PORT);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr));

    char buffer[1024];
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    // Boucle qui tourne tant que l'application principale n'est pas fermée
    while (application_en_cours) {
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&sender_addr, &sender_len);
        
        if (bytes > 0) {
            buffer[bytes] = '\0'; // Toujours bien fermer la chaîne
            std::lock_guard<std::mutex> lock(mutex_messages);
            Messages.push(messages({std::string(inet_ntoa(sender_addr.sin_addr)), std::string(buffer)}));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    close(sock);
}

// --- LE THREAD DE GESTION DES MESSAGES ---
void manage_answer() {
    while (application_en_cours)
    {
        messages msg_a_traiter;       // Pour copier le message
        bool j_ai_un_message = false; // Pour savoir si on doit traiter quelque chose

        // Gestion sécurisé de l'accès au vecteur Messages
        { 
            std::lock_guard<std::mutex> lock(mutex_messages);
            
            if( !Messages.empty() ) 
            {
                msg_a_traiter = Messages.front(); // On copie
                Messages.pop();                   // On supprime de la file
                j_ai_un_message = true;
            }
        } 

        if (j_ai_un_message) 
        {   
            // Dechiffrage
            std::string data = std::string(msg_a_traiter.data);
            data = let_unxor_msg(data);

            // Découpage du message
            std::string commande, id_paquet;
            int splitIndex = data.find('\n');

            if (splitIndex != std::string::npos) {
                commande = data.substr(0, splitIndex);
                id_paquet = data.substr(splitIndex + 1);
            }
            else {
                commande = data;
                id_paquet = ""; 
            }

            commande = trim(commande);
            id_paquet = trim(id_paquet);

            // Traitement du message
            // std::cout << "\n[Reçu de " << msg_a_traiter.sender_ip << "] : " << commande << std::endl;

            if (commande == "I_AM_DRONE") {
                std::cout << "-> Le drone s'est identifié ! Il a pour IP : " << msg_a_traiter.sender_ip << std::endl;
                _ip_drone = msg_a_traiter.sender_ip;
                drone_trouve = true;
            }
            else if (commande.starts_with("LEFT_")) {
                std::string payload = commande.substr(std::string("LEFT_").length());
                if (payload == "OK") std::cout << "Le drone est allé à gauche" << std::endl;
                else if (payload == "NOK") std::cout << "Le drone n'a pas pu aller à gauche" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("RIGHT_")) {
                std::string payload = commande.substr(std::string("RIGHT_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("UP_")) {
                std::string payload = commande.substr(std::string("UP_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("DOWN_")) {
                std::string payload = commande.substr(std::string("DOWN_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("BACKWARD_")) {
                std::string payload = commande.substr(std::string("BACKWARD_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("FORWARD_")) {
                std::string payload = commande.substr(std::string("FORWARD_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("DO_A_FLIP_")) {
                std::string payload = commande.substr(std::string("DO_A_FLIP_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("TAKEOFF_")) {
                std::string payload = commande.substr(std::string("TAKEOFF_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("LAND_")) {
                std::string payload = commande.substr(std::string("LAND_").length());
                if (payload == "OK") std::cout << "XXXX" << std::endl;
                else if (payload == "NOK") std::cout << "XXXXX" << std::endl;
                else std::cout << "Erreur payload" << std::endl;
            }
            else if (commande.starts_with("BATTERY_")) {
                std::string payload = commande.substr(std::string("BATTERY_").length());
                // TODO
            }
            else if (commande.starts_with("ALTITUDE_")) {
                std::string payload = commande.substr(std::string("ALTITUDE_").length());
                // TOdo
            }
            else if (commande.starts_with("YAW_AND_PITCH_")) {
                std::string payload = commande.substr(std::string("YAW_AND_PITCH_").length());
                // Todo
            }
            else if (commande.starts_with("POSITION_")) {
                std::string payload = commande.substr(std::string("POSITION_").length());
                // Todo
            }
            else if (commande.starts_with("EMERGENCY_STOP")) {
                std::string payload = commande.substr(std::string("EMERGENCY_STOP").length());
                // Todo
            }
            else {
                std::cout << "-> Commande inconnue ou donnée télémétrique. (" << commande << ")" << std::endl;
            }
            if (!id_paquet.empty()) delete_paquet_id(std::stoi(id_paquet));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Crée un id de paquet (empèche les doubons)
int generate_paquet_id() {
    int random_id;

    do {
        random_id = std::rand();
    } while (std::find(waiting_awsers_paquet_id.begin(), waiting_awsers_paquet_id.end(), random_id) != waiting_awsers_paquet_id.end());

    // Ajout du nouvel ID unique dans la liste
    waiting_awsers_paquet_id.push_back(random_id);

    return random_id;
}

// Supprime l'id de la liste des id de paquet attendu
void delete_paquet_id(int paquet_id) {
    waiting_awsers_paquet_id.erase(
        std::remove(waiting_awsers_paquet_id.begin(), 
                    waiting_awsers_paquet_id.end(), 
                    paquet_id),
        waiting_awsers_paquet_id.end()
    );
}

std::string base64_encode(const std::string& input) {
    std::string output;
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

std::string base64_decode(const std::string& input) {
    int T[256];
    for (int i = 0; i < 256; i++) T[i] = -1;
    for (int i = 0; i < 64; i++) T[(unsigned char)b64_chars[i]] = i;

    std::string output;
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

std::string let_xor_msg(const std::string& msg) {
    std::string xored;
    xored.reserve(msg.length());

    for (size_t i = 0; i < msg.length(); i++) {
        unsigned char a = (unsigned char)private_key[i % private_key.length()];
        unsigned char b = (unsigned char)msg[i];
        xored += char(a ^ b);
    }

    return base64_encode(xored);
}

std::string let_unxor_msg(const std::string& encoded_msg) {
    std::string xored = base64_decode(encoded_msg);
    std::string msg;
    msg.reserve(xored.length());

    for (size_t i = 0; i < xored.length(); i++) {
        unsigned char a = (unsigned char)private_key[i % private_key.length()];
        unsigned char b = (unsigned char)xored[i];
        msg += char(a ^ b);
    }

    return msg;
}

// Nettoie les espaces et caractères invisibles au début et à la fin
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}