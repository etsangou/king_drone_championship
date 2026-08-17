#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <cmath>
#include <iomanip>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <GL/freeglut.h>

// --- VARIABLES GLOBALES PARTAGÉES ---
std::mutex imu_mutex;
float g_yaw = 0.0f, g_roll = 0.0f, g_pitch = 0.0f;
float g_ax = 0.0f, g_ay = 0.0f, g_az = 0.0f;

// --- PARAMÈTRES DE CAMÉRA & UI ---
float camAngleX = 45.0f;
float camAngleY = 45.0f;
float camDistance = 8.0f;
bool showMenu = true;
bool showWorldAxes = true;

// --- THREAD DE LECTURE SÉRIE ---
void serialThreadFunc() {
    const char* portName = "/dev/ttyUSB0";
    int serial_port = open(portName, O_RDWR | O_NOCTTY | O_SYNC);

    if (serial_port < 0) {
        std::cerr << "[ERREUR] Impossible d'ouvrir le port série " << portName << std::endl;
        return;
    }

    struct termios tty;
    tcgetattr(serial_port, &tty);
    tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB; tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS; tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    tty.c_oflag &= ~(OPOST | ONLCR);
    tty.c_cc[VTIME] = 10; tty.c_cc[VMIN] = 0;
    
    // Configuré pour 115200 baud
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tcsetattr(serial_port, TCSANOW, &tty);

    std::string buffer;
    char read_buf[256];

    while (true) {
        int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
        if (num_bytes > 0) {
            for (int i = 0; i < num_bytes; ++i) {
                char c = read_buf[i];
                if (c == '\n') {
                    if (!buffer.empty() && buffer.back() == '\r') buffer.pop_back();
                    
                    std::stringstream ss(buffer);
                    std::string token;
                    std::vector<std::string> values;
                    while (std::getline(ss, token, ',')) values.push_back(token);
                    
                    if (values.size() == 6) {
                        try {
                            std::lock_guard<std::mutex> lock(imu_mutex);
                            g_yaw   = std::stof(values[0]);
                            g_roll  = std::stof(values[1]);
                            g_pitch = std::stof(values[2]);
                            g_ax    = std::stof(values[3]);
                            g_ay    = std::stof(values[4]);
                            g_az    = std::stof(values[5]);
                        } catch (...) {}
                    }
                    buffer.clear();
                } else {
                    buffer += c;
                }
            }
        }
    }
    close(serial_port);
}

// --- FONCTIONS DE DESSIN 3D ---

// Dessine les axes orthogonaux (Rouge=X, Vert=Y, Bleu=Z)
void drawAxes(float scale) {
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    // Axe X (Rouge - Avant)
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(scale, 0, 0);
    // Axe Y (Vert - Gauche)
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, scale, 0);
    // Axe Z (Bleu - Haut)
    glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, scale);
    glEnd();
    glEnable(GL_LIGHTING);
}

// Dessine un cylindre (utilisé pour les moteurs)
void drawCylinder(float radius, float height) {
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, radius, radius, height, 16, 16);
    gluDeleteQuadric(quad);
}

// Dessine un drone professionnel procédural
void drawDroneModel() {
    // Matériau du châssis (Gris foncé mat)
    GLfloat mat_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat mat_diffuse[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

    // Corps central
    glPushMatrix();
    glScalef(1.0f, 0.8f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Repère visuel "Nez" du drone (Rouge)
    glPushMatrix();
    GLfloat red_diffuse[] = { 0.8f, 0.1f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, red_diffuse);
    glTranslatef(0.55f, 0.0f, 0.0f);
    glutSolidSphere(0.15f, 16, 16);
    glPopMatrix();

    // Configuration des 4 bras et moteurs
    float armLength = 1.2f;
    float angles[] = { 45.0f, 135.0f, 225.0f, 315.0f };

    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glRotatef(angles[i], 0.0f, 0.0f, 1.0f);
        
        // Bras (Gris clair)
        GLfloat arm_diffuse[] = { 0.6f, 0.6f, 0.6f, 1.0f };
        glMaterialfv(GL_FRONT, GL_DIFFUSE, arm_diffuse);
        glPushMatrix();
        glTranslatef(armLength / 2.0f, 0.0f, 0.0f);
        glScalef(armLength, 0.1f, 0.1f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Moteur (Cylindre noir)
        GLfloat motor_diffuse[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        glMaterialfv(GL_FRONT, GL_DIFFUSE, motor_diffuse);
        glTranslatef(armLength, 0.0f, -0.1f);
        drawCylinder(0.15f, 0.3f);

        // Hélices (Disques semi-transparents bleutés)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GLfloat prop_diffuse[] = { 0.5f, 0.8f, 1.0f, 0.3f }; // Alpha = 0.3
        glMaterialfv(GL_FRONT, GL_DIFFUSE, prop_diffuse);
        glTranslatef(0.0f, 0.0f, 0.35f);
        
        GLUquadric* disk = gluNewQuadric();
        gluDisk(disk, 0.05, 0.6, 20, 1);
        gluDeleteQuadric(disk);
        glDisable(GL_BLEND);

        glPopMatrix();
    }
}

// --- FONCTIONS HUD (INTERFACE 2D) ---

// Permet de dessiner du texte 2D facilement
void renderText(float x, float y, void* font, const std::string& text, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Convertit un float en string avec X décimales
std::string fmt(float val) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << val;
    return stream.str();
}

// Dessine l'interface utilisateur superposée
void drawHUD(int width, int height) {
    // Sauvegarder les matrices 3D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0); // Repère 2D (0,0 en haut à gauche)
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Dessiner le fond du panneau latéral
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Noir transparent
    glBegin(GL_QUADS);
    glVertex2f(10, 10);
    glVertex2f(280, 10);
    glVertex2f(280, 240);
    glVertex2f(10, 240);
    glEnd();
    glDisable(GL_BLEND);

    // Récupérer les données
    float y, r, p, ax, ay, az;
    {
        std::lock_guard<std::mutex> lock(imu_mutex);
        y = g_yaw; r = g_roll; p = g_pitch;
        ax = g_ax; ay = g_ay; az = g_az;
    }

    // Titres et Données
    void* fontBig = GLUT_BITMAP_HELVETICA_18;
    void* fontSmall = GLUT_BITMAP_HELVETICA_12;

    renderText(20, 40, fontBig, "IMU DASHBOARD", 0.3f, 0.8f, 1.0f);
    
    // Orientation
    renderText(20, 70, fontSmall, "ORIENTATION (deg)", 0.6f, 0.6f, 0.6f);
    renderText(20, 90, fontBig, "YAW:   " + fmt(y), 1.0f, 1.0f, 1.0f);
    renderText(20, 110, fontBig, "PITCH: " + fmt(p), 1.0f, 1.0f, 1.0f);
    renderText(20, 130, fontBig, "ROLL:  " + fmt(r), 1.0f, 1.0f, 1.0f);

    // Accélération
    renderText(20, 160, fontSmall, "ACCELERATION (m/s2)", 0.6f, 0.6f, 0.6f);
    renderText(20, 180, fontBig, "X: " + fmt(ax), 1.0f, 0.4f, 0.4f);
    renderText(20, 200, fontBig, "Y: " + fmt(ay), 0.4f, 1.0f, 0.4f);
    renderText(20, 220, fontBig, "Z: " + fmt(az), 0.4f, 0.6f, 1.0f);

    // Menu d'aide (en bas à gauche)
    if (showMenu) {
        renderText(10, height - 80, fontSmall, "CONTROLES :", 1.0f, 1.0f, 0.0f);
        renderText(10, height - 60, fontSmall, "[Fleches] Tourner la camera", 1.0f, 1.0f, 1.0f);
        renderText(10, height - 40, fontSmall, "[+/-] Zoomer/Dezoomer", 1.0f, 1.0f, 1.0f);
        renderText(10, height - 20, fontSmall, "[A] Toggle Axes Monde   [M] Toggle Menu   [Q] Quitter", 1.0f, 1.0f, 1.0f);
    } else {
        renderText(10, height - 20, fontSmall, "[M] Afficher Menu", 0.6f, 0.6f, 0.6f);
    }

    // Restaurer les matrices 3D
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// --- BOUCLE PRINCIPALE DE RENDU ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 1. Calcul de la position de la caméra
    float camX = camDistance * cos(camAngleY * M_PI / 180.0f) * cos(camAngleX * M_PI / 180.0f);
    float camY = camDistance * cos(camAngleY * M_PI / 180.0f) * sin(camAngleX * M_PI / 180.0f);
    float camZ = camDistance * sin(camAngleY * M_PI / 180.0f);
    gluLookAt(camX, camY, camZ, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    // 2. Axes du monde absolu
    if (showWorldAxes) {
        drawAxes(10.0f);
    }

    // 3. Transformations du Drone
    float y, r, p, ax, ay, az;
    {
        std::lock_guard<std::mutex> lock(imu_mutex);
        y = g_yaw; r = g_roll; p = g_pitch;
        ax = g_ax; ay = g_ay; az = g_az;
    }

    glPushMatrix();
    
    // Application des rotations (Ordre à adapter selon ton Arduino)
    glRotatef(y, 0.0f, 0.0f, 1.0f); // Lacet
    glRotatef(p, 0.0f, 1.0f, 0.0f); // Tangage
    glRotatef(r, 1.0f, 0.0f, 0.0f); // Roulis

    // Axes locaux du drone
    drawAxes(2.0f);

    // Dessin du Drone
    drawDroneModel();

    // Vecteur d'accélération propre au drone
    glDisable(GL_LIGHTING);
    glLineWidth(5.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 0.0f); // Jaune vif
    glVertex3f(0.0f, 0.0f, 0.0f); 
    glVertex3f(ax * 0.1f, ay * 0.1f, az * 0.1f);
    glEnd();
    glEnable(GL_LIGHTING);

    glPopMatrix();

    // 4. Interface utilisateur 2D (HUD)
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    drawHUD(width, height);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

// --- GESTION DES CONTRÔLES ---
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'q': case 'Q': case 27: exit(0); break; // 27 = Echap
        case 'm': case 'M': showMenu = !showMenu; break;
        case 'a': case 'A': showWorldAxes = !showWorldAxes; break;
        case '+': camDistance -= 0.5f; if (camDistance < 2.0f) camDistance = 2.0f; break;
        case '-': camDistance += 0.5f; if (camDistance > 20.0f) camDistance = 20.0f; break;
    }
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:  camAngleX -= 5.0f; break;
        case GLUT_KEY_RIGHT: camAngleX += 5.0f; break;
        case GLUT_KEY_UP:    camAngleY += 5.0f; if (camAngleY > 89.0f) camAngleY = 89.0f; break;
        case GLUT_KEY_DOWN:  camAngleY -= 5.0f; if (camAngleY < -89.0f) camAngleY = -89.0f; break;
    }
}

int main(int argc, char** argv) {
    // Thread série
    std::thread serialThread(serialThreadFunc);
    serialThread.detach();

    // Init OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE); // Anti-aliasing
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Dashboard Drone IMU Pro");

    // Paramètres OpenGL de base
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // Gris anthracite

    // Configuration d'une lumière douce
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat light_pos[] = { 10.0f, 10.0f, 10.0f, 1.0f };
    GLfloat light_amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_dif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif);

    // Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}