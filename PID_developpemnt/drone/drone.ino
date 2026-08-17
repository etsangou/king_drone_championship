// Programme qui simule un MPU6500 avec extraction de l'accélération linéaire (mouvement pur)

const unsigned long SEND_INTERVAL = 50; // 50 ms (20 FPS) pour une belle fluidité 3D
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A0));
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SEND_INTERVAL) {
    previousMillis = currentMillis;

    // Le temps en secondes pour animer nos mathématiques
    float t = currentMillis / 1000.0;

    // 1. SIMULATION DU VOL (Angles)
    // Le drone fait des manœuvres fluides
    float pitch_f = 30.0 * sin(t * 2.0);                  // Tangage (Nez qui plonge et remonte)
    float roll_f  = 20.0 * cos(t * 1.3);                  // Roulis (Mouvement gauche/droite)
    float yaw_f   = 45.0 * sin(t * 0.5);                  // Lacet (Rotation globale)

    int yaw   = (int)yaw_f;
    int roll  = (int)roll_f;
    int pitch = (int)pitch_f;

    float vibration_x = random(-2, 3) / 10.0; 
    float vibration_y = random(-2, 3) / 10.0;
    float vibration_z = random(-6, 7) / 10.0;

    float ax = (15.0 * sin(pitch_f * PI / 180.0)) + vibration_x;
    float ay = (-15.0 * sin(roll_f  * PI / 180.0)) + vibration_y;
    float az = (3.0 * sin(t * 4.0)) + vibration_z;

    Serial.print(yaw);
    Serial.print(",");
    Serial.print(roll);
    Serial.print(",");
    Serial.print(pitch);
    Serial.print(",");
    Serial.print(ax);
    Serial.print(",");
    Serial.print(ay);
    Serial.print(",");
    Serial.println(az);
  }
}