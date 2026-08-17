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
#include <queue>
// #include <MPU6500_WE> // Pour la lecture des données gyroscopiques et acelerométrique

class drive
{
private:
    String _battery = "70"; // %
    String _altitude = "280"; // m oar rappoer au niveau de la mer
    String _yawAndPitch = "180_270"; // ° par rapport au Nord
    String _position = "48.8584, 2.2945"; // GPS decimal
    TaskHandle_t _multithread = NULL;

public:
    drive(/* args */);
    ~drive();

    //void stabilize();
    int left();
    int right();
    int up();
    int down();
    int forward();
    int backward();
    int doAFlip();
    int takeoff();
    int land();
    int powerOnSelfTest();
    String getBattery();
    String getAltidude();
    String getYawAndPitch();
    String getPosition();
    int emergencyStop();
    static void flyManager(void * pvParameters);
};