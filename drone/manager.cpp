/**
    @file manager.cpp
    @brief Classe de gestion du drone
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#include "manager.hpp"

manager::manager() {
    _flyManager = new drive();
}

//* TODO
manager::~manager() {
    this->stop(); // On eteint tout les treads
    Serial.println("Arrêt complet du drone...");
    Serial.flush(); // S'attend à ce que le message soit bien envoyé
}

void manager::initProtocol() {
    Serial.println("Initialisation et autotest des composants...");
    if (_flyManager->powerOnSelfTest()) {
        Serial.println("L'un des composants present une erreur opérationnel.");
        return;
    }
    _isOK = true;
    Serial.println("Tous les composants sont opérationnels.");
    return;
}

void manager::initWirelessConnexion() {
    // Deconnection d'un ancien réseau potentiel
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(1000);

    // Tentative de connexion
    WiFi.begin(ssid, password);

    // Attente de la connexion
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    // Echec que la connexion ?
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("SSID ou WIFI invalide ou non trouvé");
        return;
    }

    // Réussite de la connexion
    Serial.print("@IP du drone : "); Serial.println(WiFi.localIP());

    return;
}

void manager::waitingPCCall() {
    Serial.println("Attente du message d'identification du pc");
    while (!_personalComputerIPKnowed) delay(500);
    return; 
}

void manager::sendData(String msg) {
    if(_personalComputerIPKnowed) {
        _udp.beginPacket(_personalComputerIP, OUT_COMMUNICATION);
        _udp.print(security::let_xor_msg(msg) + "\r\n");
        _udp.endPacket();
    }
}

void manager::startListening() {
    _udp.begin(IN_COMMUNICATION);
    Serial.printf("Écoute UDP démarrée sur le port %d\n", IN_COMMUNICATION);
    _appStart = true;

    xTaskCreatePinnedToCore(
        manager::answerMannager,  // Function to implement the task
        "answerMannager",         // Name of the task
        4096,                     // Stack size in words
        this,                     // Task input parameter
        1,                        // Priority of the task
        &_multithread,            // Task handle to store the reference
        0                         // Core ID (0 or 1)
    );
    return;
}

//* TODO
void manager::start() {
    // Vérification des conditions d'amorçages
    if (!_isOK && !(WiFi.status() != WL_CONNECTED) && !_personalComputerIPKnowed) {
        Serial.println("Condition de démmarage non valide");
        return;
    }

    // Attente d'instruction
    Serial.println("Lancement du Fly manager");

    _flyManager->start();
    _flyManager->hovering();

    for(;;) { 
        static paquet messageToTreat;
        bool isNewMessage = false;

        // Lecture du la file Messages
        if( !_messagesQueue.empty() ) {
            messageToTreat = _messagesQueue.front(); // On copie
            _messagesQueue.pop();
            isNewMessage = true;
        }
        String payload = DEFAULT_OUT_CMD;

        if (isNewMessage) {
            if (messageToTreat.payload == commands::LEFT) payload = "LEFT_" + String(_flyManager->left() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::RIGHT) payload = "RIGHT_" + String(_flyManager->right() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::UP) payload = "UP_" + String(_flyManager->up() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::DOWN) payload = "DOWN_" + String(_flyManager->down() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::BACKWARD) payload = "BACKWARD_" + String(_flyManager->backward() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::FORWARD) payload = "FORWARD_" + String(_flyManager->forward() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::DO_A_FLIP) payload = "DO_A_FLIP_" + String(_flyManager->doAFlip() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::TAKEOFF) payload = "TAKEOFF_" + String(_flyManager->takeoff() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::LAND) payload = "LAND_" + String(_flyManager->land() ? "NOK" : "OK");
            else if (messageToTreat.payload == commands::GET_BATTERY) payload = "BATTERY_" + _flyManager->getBattery();
            else if (messageToTreat.payload == commands::GET_ALTITUDE) payload = "ALTITUDE_" + _flyManager->getAltidude();
            else if (messageToTreat.payload == commands::GET_YAW_AND_PITCH) payload = "YAW_AND_PITCH_" + _flyManager->getYawAndPitch();
            else if (messageToTreat.payload == commands::GET_POSITION) payload = "POSITION_" + _flyManager->getPosition();
            else Serial.println("Commande non géré");
            
            //Serial.println("Out msg : payload ->" + (payload.isEmpty() ? messageToTreat.payload : payload) + ", pID -> " + messageToTreat.paquetID);
            sendData(payload + "\n" + messageToTreat.paquetID);
        }

        delay(20);
    }

    delete _flyManager;
    _flyManager = nullptr;
}

//* TODO
void manager::stop() {
    if (_multithread != NULL) {
        vTaskDelete(_multithread); 
        _multithread = NULL;
    }
    if (_flyManager != nullptr) {
        delete _flyManager;
        _flyManager = nullptr;
    }
    return;
}

void manager::setPersonalComputerIP(IPAddress newIP) {
    _personalComputerIP = newIP;
    _personalComputerIPKnowed = true;
    Serial.println(String("@IP du PC reçu : ") + _personalComputerIP.toString());
}

void manager::answerMannager(void * pvParameters) {
    manager* instance = static_cast<manager*>(pvParameters);

    // Ajoute à la file les nouvelles commande et repond au pc avec un ACK
    while (instance->_appStart)
    {
        int packetSize = instance->_udp.parsePacket();
        
        if (packetSize) {
            int len = instance->_udp.read(instance->_packetBuffer, 255);
            if (len > 0) instance->_packetBuffer[len] = 0;

            // Dechiffrage du message
            String data = security::let_unxor_msg(String(instance->_packetBuffer));

            // Découpage du message
            String command, id_paquet;
            int splitIndex = data.indexOf('\n');

            if (splitIndex != -1) {
                command = data.substring(0, splitIndex);
                id_paquet = data.substring(splitIndex + 1);
            }
            else {
                command = data;
                id_paquet = ""; 
            }

            // Affichage de la commande
            // Serial.print("\nCommande reçu : ");
            // Serial.println(command);

            /// --- Gestion des messages --- ///
            if (command == commands::EMERGENCY_STOP) {
                // Gestion de l'arrêt d'urgence
                Serial.println("Arrêt d'urgence lancé");
                instance->_flyManager->emergencyStop();
                instance->sendData("EMERGENCY_STOP\n" + id_paquet);
            }
            else if (command == commands::WHO_IS_DRONE_OF_KING) {
                // On mémorise l'IP du PC pour pouvoir lui envoyer la télémétrie plus tard !
                instance->setPersonalComputerIP(instance->_udp.remoteIP());
                instance->sendData("I_AM_DRONE");
            }
            else {
                if (commands::isValid(command)) instance->addCommandToQueue(command, id_paquet);
                else Serial.println("Commande inconnue : " + command);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

//* TODO : Ajoute la sécurisation de l'accès à la list
void manager::addCommandToQueue(String command, String paquetID) {
    _messagesQueue.push(paquet{command, paquetID});
    Serial.println("La commande : " + command + " vient d'être ajouté à la liste.");
    //Serial.println("Envoie de : " + command + "\n" + paquetID);
}