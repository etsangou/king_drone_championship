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
    if (_mpu6500 != NULL) {
        delete _mpu6500; 
        _mpu6500 = nullptr;
    }
    return;
}

int drive::powerOnSelfTest() {
    // On initialise les capteurs accelerométrique et gyroscopique
    // startMPU6500();

    // On configure les broches moteurs en sortie
    pinMode(PIN_MOTEUR_A1, OUTPUT);
    pinMode(PIN_MOTEUR_A2, OUTPUT);
    pinMode(PIN_MOTEUR_A3, OUTPUT);
    pinMode(PIN_MOTEUR_A4, OUTPUT);

    // On les mets les moteurs à l'arrêt
    analogWrite(PIN_MOTEUR_A1, 0);
    analogWrite(PIN_MOTEUR_A2, 0);
    analogWrite(PIN_MOTEUR_A3, 0);
    analogWrite(PIN_MOTEUR_A4, 0);

    // On ouvre un thread pour le FlyManager
    xTaskCreatePinnedToCore(
        drive::flyManager,  // Function to implement the task
        "flyManager",       // Name of the task
        4096,               // Stack size in words
        this,               // Task input parameter
        1,                  // Priority of the task
        &_multithread,      // Task handle to store the reference
        0                   // Core ID (0 or 1)
    );
    return 0;
}

void drive::flyManager(void * pvParameters) {
    drive* instance = static_cast<drive*>(pvParameters);

    // Attend l'autorisation de decolage
    while (!instance->_started) { delay(500); }

    Serial.println("AHHH");

    for (;;) {
        // On calcule delat t
        instance->findDelatT();

        // On lit les données du MPU6500 (accéléromètre + gyroscope)
        instance->readMPU6500();

        // On estimatime l'angle d'inclinaison actuel du drone
        instance->calculateAxes();

        // On calcul la correction en fonction de l'ordre actuel (PID)
        instance->calculatePID();
        
        // On applique les corrections
        instance->applyCorrection();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

int drive::start() {
    _started = true;
    return 0;
}




// Méthode de gestion
int drive::hovering() {
    _pitchOrder = 0;
    _rollOrder = 0;
    return 0;
}

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

int drive::emergencyLand(){
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

int drive::startMPU6500() {
    Wire.begin(); // Bus I2C
    _mpu6500 = new MPU6500_WE(MPU6500_ADDR);

    // 1. Initialisation de la communication
    if (!_mpu6500->init()) {
        Serial.println("Erreur : MPU6500 introuvable. Vérifie le câblage I2C !");
        while (1);
    }

    Serial.println("MPU6500 connecté !");

    // 2. Calibration automatique au repos
    Serial.println("Ne bouge pas le drone, calibration en cours...");
    delay(1000);
    _mpu6500->autoOffsets(); // Calcule et soustrait les offsets matériels
    Serial.println("Calibration terminée !");

    // 3. Configuration des plages de mesure (optionnel mais recommandé)
    
    _mpu6500->setAccRange(MPU6500_ACC_RANGE_4G); // Plage accéléromètre : ±2g, ±4g, ±8g ou ±16g
    _mpu6500->setGyrRange(MPU6500_GYRO_RANGE_500); // Plage gyroscope : ±250°/s, ±500°/s, ±1000°/s ou ±2000°/s

    // Filtre passe-bas matériel interne (DLPF) pour atténuer le bruit moteur
    _mpu6500->enableGyrDLPF();                  // Active le filtre pour le gyroscope
    _mpu6500->setGyrDLPF(MPU6500_DLPF_6);       // Applique le niveau 6
    
    _mpu6500->enableAccDLPF(true);              // Active le filtre pour l'accéléromètre
    _mpu6500->setAccDLPF(MPU6500_DLPF_6);       // Applique le niveau 6
}

int drive::readMPU6500() {
    //_acc = _mpu6500->getGValues(); // Lecture de l'accélération (en g)
    //_gyro = _mpu6500->getGyrValues(); // Lecture de la vitesse angulaire (en °/s)

    return 0;
}

int drive::calculateAxes() {
    float alpha = TAU / (TAU + _dt);

    float pitch_acc = atan2(
        _acc.x , ( sqrt(_acc.y*_acc.y) + (_acc.z*_acc.z) ) 
    ) * (180.0 / PI); // Altération de l'axe Pitch en degré par l'accélération

    float roll_acc = atan2(
        _acc.y , sqrt( (_acc.x*_acc.x) + (_acc.z*_acc.z) ) 
    ) * (180.0 / PI); // Altération de l'axe Roll en degré par l'accélération


    _pitch = alpha * (_pitch_acc_m1 + (_acc.y * _dt)) + (1 - alpha) * pitch_acc;
    _roll = alpha * (_roll_acc_m1 + (_acc.x * _dt)) + (1 - alpha) * roll_acc;  

    // On met à jour les valeurs pour les prochaines boucles
    _pitch_acc_m1 = pitch_acc;
    _roll_acc_m1 = roll_acc;

    return 0;
}

int drive::calculatePID() {
    // Calcule de PID sur Pitch, Roll et Yaw
    _pitchCommand = PID(_pitch, _pitch_m1, _pitchOrder, _kpPitch, _kiPitch, _kdPitch, _pitch_I);
    _rollCommand = PID(_roll, _roll_m1, _rollOrder, _kpRoll, _kiRoll, _kdRoll, _roll_I);
    _yawCommand = PID(_yaw, _yaw_m1, _yawOrder, _kpYaw, _kiYaw, _kdYaw, _yaw_I);

    return 0;
}

int drive::PID(float& axe, float& previousAxe, float& axeOrder, float& kPresent, float& kIntegral, float& kDerivate, float& sumIntegral) {
    // Calcul de l'erreur
    float erreur = axeOrder - axe;

    // Action Proportionnelle
    float P = kPresent * erreur;

    // Action Intégrale (avec anti-windup pour éviter la saturation)
    // I = 0 tant que le drone est posé et que les gaz sont coupés.
    sumIntegral = sumIntegral + (kIntegral * erreur * _dt);
    sumIntegral = constrain(sumIntegral, -50.0f, 50.0f);

    // Action Dérivée (calculée sur la mesure pour éviter les à-coups)
    float D = kDerivate * (axe - previousAxe) / _dt;

    // On met à jour la mémoire de l'axe pour le prochain calcul de la dérivée
    previousAxe = axe;
    
    return P + sumIntegral - D;
}

int drive::applyCorrection() {
    _rotorA1Speed = _throttle + _rollCommand + _pitchCommand - _yawCommand; // Avant gauche
    _rotorA2Speed = _throttle - _rollCommand + _pitchCommand + _yawCommand; // Avant droite
    _rotorA3Speed = _throttle + _rollCommand - _pitchCommand + _yawCommand; // Arrière gauche
    _rotorA4Speed = _throttle - _rollCommand - _pitchCommand - _yawCommand; // Arrière droite

    updateRotorSpeed();

    return 0;
}

int drive::updateRotorSpeed() {
    analogWrite(PIN_MOTEUR_A1, (int)constrain(_rotorA1Speed, 0, 255));
    analogWrite(PIN_MOTEUR_A2, (int)constrain(_rotorA2Speed, 0, 255));
    analogWrite(PIN_MOTEUR_A3, (int)constrain(_rotorA3Speed, 0, 255));
    analogWrite(PIN_MOTEUR_A4, (int)constrain(_rotorA4Speed, 0, 255));

    return 0;
}

inline int drive::findDelatT() {
    _actualTime = micros();
    float dt = (_actualTime - _previousTime) / 1000000.0f; 
    _previousTime = _actualTime;
    _dt = dt;
    return 0;
}