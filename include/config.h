#pragma once

#include <stdint.h>

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

// Sens des moteurs. Convention du code :
//  - ascension droite : pas positifs = sens du suivi (le tube va vers l'ouest) ;
//  - déclinaison : pas positifs = la déclinaison monte.
// Mettre true si un moteur tourne à l'envers de cette convention.
constexpr bool AD_DIR_INVERT = false;
constexpr bool DEC_DIR_INVERT = false;

constexpr double STEPS_PER_AXIS_TURN = double(MOTOR_STEPS) * MICROSTEPS * RAPPORT_POULIE;
constexpr double DEG_PER_STEP = 360.0 / STEPS_PER_AXIS_TURN;

// --- Vitesses --------------------------------------------------------------
constexpr int RPM = 120;              // vitesse moteur en pointage rapide
constexpr int MOTOR_ACCEL = 2000;     // accélération, en pas par seconde²

constexpr uint32_t SLEW_SPEED_HZ = uint32_t(RPM) * MOTOR_STEPS * MICROSTEPS / 60;

// Suivi : l'axe d'ascension droite fait un tour par jour sidéral.
constexpr double SIDEREAL_DAY_S = 86164.0905;
constexpr double SIDEREAL_DEG_PER_S = 360.0 / SIDEREAL_DAY_S;
constexpr uint32_t TRACKING_STEP_US = uint32_t(SIDEREAL_DAY_S * 1e6 / STEPS_PER_AXIS_TURN);

// --- Pointage --------------------------------------------------------------
// À la fin d'un pointage, le ciel a tourné pendant le trajet. Le code refait
// un petit mouvement tant que l'écart dépasse cette tolérance.
constexpr int32_t GOTO_TOLERANCE_STEPS = 2;
constexpr int GOTO_MAX_PASSES = 3;

// Position supposée au démarrage, en degrés : monture rangée, tube vers le
// pôle. Synchroniser sur une étoile connue avant le premier vrai pointage.
constexpr double START_RA = 0;
constexpr double START_DEC = 90;

// --- Lieu d'observation par défaut ----------------------------------------
constexpr double SITE_LONGITUDE = 2.35;   // degrés, est positif
constexpr double SITE_LATITUDE = 48.89;   // degrés, nord positif
constexpr double SITE_ALTITUDE = 31;      // mètres

// --- Liaisons ----------------------------------------------------------------
constexpr uint32_t DEBUG_BAUD = 115200;

// LX200 par câble : le port se choisit sur la page web (broches, USB, aucun).
constexpr uint32_t LX200_SERIAL_BAUD = 9600;  // vitesse standard LX200
constexpr int PIN_LX200_RX = 16;
constexpr int PIN_LX200_TX = 17;

// LX200 par Wi-Fi.
constexpr uint16_t LX200_TCP_PORT = 4030;
constexpr int LX200_TCP_MAX_CLIENTS = 2;
