/**
    @file drone.ino
    @brief Programme principal du drone
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#include "manager.hpp"

manager* core = nullptr;

void setup() {
  Serial.begin(115200);
  delay(3000); // Pour avoir le temps de lancer monitor (arduino cli)

  core = new manager();

  // Procedure d'initialisation du drone (avec un ckeckup de l'etat des composants)
  core->initProtocol();

  // Connexion au réseau WIFI
  core->initWirelessConnexion();

  // Démarrage de l'écoute sur le port UDP (sur un autre thread)
  core->startListening();

  // En attente du PC (On attend la commande chiffré : "WHO_IS_DRONE_OF_KING")
  core->waitingPCCall();

  // On démmare le programme de controle du drone (et d'arrêt d'urgence)
  core->start();
}

// On eteint tout et au dodo
void loop() {
  delete core;
  esp_deep_sleep_start();
}