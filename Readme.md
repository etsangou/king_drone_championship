# 🚁 Projet de Drone - Compétition Autonome

Ce dépôt contient le code source utilisé pour le drone d'Enzo lors de la compétition de drones "Enzo vs Papa". 

L'architecture du projet est divisée en deux parties distinctes :
- `drone/` : Le code embarqué sur le drone (ESP32 / C++)
- `control/` : Le code de contrôle et de télémétrie (PC)

---

## 🎯 Objectif de la compétition

L'objectif principal est de réaliser un tour de la maison (ou d'un circuit défini) de manière totalement autonome. 
Le gagnant sera celui qui aura programmé le plus de fonctionnalités avancées et stables : 
* Esquive d'obstacles
* Figures acrobatiques (flips)
* Signaux lumineux intelligents (blink)
* Optimisation de la trajectoire
* Retour à la base

---

## 📜 Les 6 Règles d'Or

Pour garantir un défi de programmation équitable, les règles suivantes s'appliquent strictement :

1. **Architecture unique :** Il n'y aura qu'un seul programme côté PC, et un seul programme embarqué sur le drone.
2. **100% Fait Maison ("From Scratch") :** Interdiction stricte d'utiliser des bibliothèques externes pour l'asservissement, la stabilisation ou le calcul des trajectoires. Le développeur doit coder ses propres mathématiques de vol.
3. **Check-up obligatoire :** La phase d'initialisation n'est validée que lorsque le drone est connecté au PC et a renvoyé un rapport d'état complet de ses systèmes.
4. **Télémétrie en temps réel :** À tout moment du vol, le drone doit envoyer ses informations de télémétrie au PC (vitesse approximative, altitude, état des capteurs, batterie, etc.).
5. **Fonctions vitales requises :** Le décollage, l'atterrissage, le déplacement basique et la procédure d'arrêt d'urgence (Kill Switch) sont obligatoires pour concourir.
6. **Autonomie totale :** Lors de l'épreuve, le drone doit être indépendant. Il ne doit se déplacer qu'en suivant ses propres algorithmes et les retours de ses capteurs, sans pilotage manuel.

---

## 🛠️ Matériel utilisé
*(À compléter)*

## Note

```bash
/* Libération du port ttyUSB0 */
sudo fuser -k /dev/ttyUSB0

/* Compilation et envoie des données sur la carte */
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 .

/* Lecture des messages */
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200

/* En cas d'erreur de televersement */
newgrp dialout
```

## Fonctionalité

- Gestion des ACKs
- Communication PC Drone par Wifi (UDP)
- Chiffrage de la communication par cléé symétrique
- Anti bloquage pour le PC (via des threads)
