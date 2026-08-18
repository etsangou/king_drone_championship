/**
    @file drive.hpp
    @brief Classe de gestion du vol
    @author Enzo Tsangouabeka
    @version v0.1
    @date 14/08/2026
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MPU6500_WE.h> // Pour la lecture des données gyroscopiques et acelerométrique

#define MPU6500_ADDR 0x68
#define PIN_MOTEUR_A1 18 // Avant-Gauche
#define PIN_MOTEUR_A2 19 // Avant-Droit
#define PIN_MOTEUR_A3 21 // Arrière-Gauche
#define PIN_MOTEUR_A4 22 // Arrière-Droit
#define TAU 0.4f // Vitesse de correstion de l'agrorithme PID

class drive
{
private:
    String _battery = "70"; // %
    String _altitude = "280"; // m oar rappoer au niveau de la mer
    String _yawAndPitch = "180_270"; // ° par rapport au Nord
    String _position = "48.8584, 2.2945"; // GPS decimal
    TaskHandle_t _multithread = NULL;

    // Ordre de controle pour l'algorithme PID -180° à 180° (180° == -180°)
    float _pitchOrder = 0.0f; 
    float _rollOrder = 0.0f;
    float _yawOrder = 0.0f;

    // Valeur calculer via le vecteur gravité
    float _pitch = 0.0f;
    float _roll = 0.0f;
    float _yaw = 0.0f;

    // Valeur à T-1
    float _pitch_m1 = 0.0f; 
    float _roll_m1 = 0.0f;
    float _yaw_m1 = 0.0f;

    // Les résultats du calcul PID
    float _pitchCommand = 0.0f;
    float _rollCommand  = 0.0f;
    float _yawCommand   = 0.0f;

    // Les accumulateurs pour l'action Intégrale (I)
    float _pitch_I = 0.0f;
    float _roll_I  = 0.0f;
    float _yaw_I   = 0.0f;

    // Constante de correction
    float _kpPitch, _kiPitch, _kdPitch;
    float _kpRoll, _kiRoll, _kdRoll;
    float _kpYaw, _kiYaw, _kdYaw;

    // Altération des axes Pitch et Roll en degré (°) par l'accélération
    float _pitch_acc_m1 = 0, _roll_acc_m1 = 0;

    // Valeur des capteurs
    xyzFloat _acc = {0.0f, 0.0f, 0.0f};
    xyzFloat _gyro = {0.0f, 0.0f, 0.0f};

    // Vitesse des hélices
    float _rotorA1Speed = 0.0f; // Avant gauche
    float _rotorA2Speed = 0.0f; // Avant droite
    float _rotorA3Speed = 0.0f; // Arrière gauche
    float _rotorA4Speed = 0.0f; // Arrière droite

    float _dt = 0.0f;
    unsigned long _previousTime = micros();
    unsigned long _actualTime = micros();
    

    int _throttle = 150; // Commande de gaz (0 -> 255)

    bool _started = false; /// Autorisation de décolage

    // Object pour la cummunication avec l'accelerometer/gyroscope mpu6500
    MPU6500_WE* _mpu6500 = nullptr;

    // Méthode privé
    int startMPU6500();
    int readMPU6500();
    int calculateAxes();
    int calculatePID();
    inline int findDelatT();
    int applyCorrection();
    int updateRotorSpeed();
    int PID(float& axe, float& previousAxe, float& axeOrder, float& kPresent, float& kIntegral, float& kDerivate, float& sumIntegral);

public:
    drive(/* args */);
    ~drive();

    // Gestion des consignes
    int left();
    int right();
    int up();
    int down();
    int forward();
    int backward();

    // Vol sur place
    int hovering();

    // Decolage et aterrisage
    int takeoff();
    int land();

    // Commandes de style
    int doAFlip();

    // Gestion
    int start();
    int powerOnSelfTest();
    int emergencyStop();
    int emergencyLand();
    

    // Telemetrie
    String getBattery();
    String getAltidude();
    String getYawAndPitch();
    String getPosition();

    // Boucle de vol
    static void flyManager(void * pvParameters);
};