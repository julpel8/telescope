#pragma once

// Réglages fixes du télescope. Changer ici, puis recompiler.

// --- Broches ESP32 ---------------------------------------------------------
// Les broches 34 à 39 sont en entrée seule : ne pas les utiliser pour un moteur.
constexpr int PIN_AD_DIR = 25;
constexpr int PIN_AD_STEP = 26;
constexpr int PIN_DEC_DIR = 32;
constexpr int PIN_DEC_STEP = 33;

// Pas pilotées par le code pour l'instant.
constexpr int PIN_LED = 4;
constexpr int PIN_MS1 = 22;
constexpr int PIN_MS2 = 21;
constexpr int PIN_MS3 = 23;

// --- Mécanique (monture HEQ5) ---------------------------------------------
constexpr int MOTOR_STEPS = 200;      // pas par tour du moteur (1,8° par pas)
constexpr int MICROSTEPS = 16;        // micro-pas réglés sur les A4988
constexpr int RAPPORT_POULIE = 216;   // tours moteur pour un tour d'axe

// --- Vitesses --------------------------------------------------------------
constexpr int RPM = 120;              // vitesse moteur en pointage rapide
constexpr int MOTOR_ACCEL = 2000;     // accélération, en pas par seconde²

// --- Lieu d'observation par défaut ----------------------------------------
constexpr double SITE_LONGITUDE = 2.35;   // degrés, est positif
constexpr double SITE_LATITUDE = 48.89;   // degrés, nord positif
constexpr double SITE_ALTITUDE = 31;      // mètres
