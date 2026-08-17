/**
    @file drive.cpp
    @brief Classe de gestion du vol
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#include "drive.hpp"

drive::drive() {
    
}

drive::~drive(){
    if (_multithread != NULL) {
        vTaskDelete(_multithread); 
        _multithread = NULL;
    }
    return;
}

int drive::powerOnSelfTest() {
    delay(500);
    // Ouverture d'un autre thread (fly_manager())
    xTaskCreatePinnedToCore(
        drive::flyManager,   // Function to implement the task
        "flyManager",         // Name of the task
        4096,             // Stack size in words
        this,             // Task input parameter
        1,                // Priority of the task
        &_multithread,     // Task handle to store the reference
        0                 // Core ID (0 or 1)
    );
    return 0;
}

void drive::flyManager(void * pvParameters) {
    for (;;) {
        /* Action à faire lors de la boucle principale :
        Lecture : Lire les données du MPU6500 (accéléromètre + gyroscope).
        Estimation : Calculer l'angle d'inclinaison actuel de ton drone via les données du MPU6500.
        Ordre : Voir quel est l'ordre en cours de suivi.
        Calcul d'erreur (Le contrôleur PID) pour trouver les vrais valeurs du (Roll, Pitch, Yaw).
        Action : Via le PID qui va donner une valeur de correction ("Sensor Fusion" (Filtre complémentaire, Filtre de Madgwick, ou Filtre de Kalman))
        */
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


// Méthode de gestion
int drive::left(){
    // A faire
    return 0;
}

int drive::right(){
    // A faire
    return 0;
}

int drive::up(){
    // A faire
    return 0;
}

int drive::down(){
    // A faire
    return 0;
}

int drive::forward(){
    // A faire
    return 0;
}

int drive::backward(){
    // A faire
    return 0;
}

int drive::doAFlip(){
    // A faire
    return 0;
}

int drive::takeoff(){
    // A faire
    return 0;
}

int drive::land(){
    // A faire
    return 0;
}

int drive::emergencyStop(){
    // A faire
    return 0;
}

// Getters
String drive::getBattery(){
    return _battery;
}

String drive::getAltidude(){
    return _altitude;
}

String drive::getYawAndPitch(){
    return _yawAndPitch;
}

String drive::getPosition(){
    return _position;
}