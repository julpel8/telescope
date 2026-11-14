#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>
#include <FastAccelStepper.h>
#include "config.h"

// Pas moteur pour un tour complet de l'axe du télescope.
#define STEPS_PER_AXIS_TURN ((double)MOTOR_STEPS * MICROSTEPS * RAPPORT_POULIE)

// Vitesse de pointage rapide, en pas par seconde.
#define SLEW_SPEED_HZ ((uint32_t)RPM * MOTOR_STEPS * MICROSTEPS / 60)

// Suivi sidéral : l'axe d'ascension droite fait un tour en un jour sidéral
// (86164 s). Intervalle entre deux pas, en microsecondes.
#define SIDEREAL_DAY_S 86164.0
#define TRACKING_STEP_US ((uint32_t)(SIDEREAL_DAY_S * 1e6 / STEPS_PER_AXIS_TURN))

/**
 * Pilote les deux moteurs avec FastAccelStepper.
 * Les pas sont produits par un périphérique matériel de l'ESP32 :
 * aucune tâche ne boucle sur le processeur pendant un mouvement.
 */
class Motors {
public:
    void initialize() {
        engine.init();

        stepper_DA = engine.stepperConnectToPin(PIN_AD_STEP);
        stepper_DEC = engine.stepperConnectToPin(PIN_DEC_STEP);
        if (!stepper_DA || !stepper_DEC) {
            Serial.println("Motors: stepperConnectToPin a échoué");
            return;
        }

        stepper_DA->setDirectionPin(PIN_AD_DIR);
        stepper_DEC->setDirectionPin(PIN_DEC_DIR);

        stepper_DA->setAcceleration(MOTOR_ACCEL);
        stepper_DEC->setAcceleration(MOTOR_ACCEL);
    }

    /** Tourne l'axe d'ascension droite d'un angle en degrés (axe, pas moteur). */
    void moveDA(double axisDegrees) {
        if (!stepper_DA) return;
        tracking = false;
        stepper_DA->setSpeedInHz(SLEW_SPEED_HZ);
        stepper_DA->move(degreesToSteps(axisDegrees));
    }

    /** Tourne l'axe de déclinaison d'un angle en degrés (axe, pas moteur). */
    void moveDEC(double axisDegrees) {
        if (!stepper_DEC) return;
        stepper_DEC->setSpeedInHz(SLEW_SPEED_HZ);
        stepper_DEC->move(degreesToSteps(axisDegrees));
    }

    /**
     * À appeler régulièrement : relance le suivi sidéral sur l'axe
     * d'ascension droite dès qu'un pointage est fini.
     */
    void updateTracking() {
        if (!stepper_DA || tracking || stepper_DA->isRunning()) return;
        stepper_DA->setSpeedInUs(TRACKING_STEP_US);
        stepper_DA->runForward();
        tracking = true;
    }

    bool isMovingDA() { return stepper_DA && stepper_DA->isRunning() && !tracking; }
    bool isMovingDEC() { return stepper_DEC && stepper_DEC->isRunning(); }

    void stopDA() {
        if (!stepper_DA) return;
        tracking = false;
        stepper_DA->stopMove();
    }

    void stopDEC() {
        if (stepper_DEC) stepper_DEC->stopMove();
    }

private:
    FastAccelStepperEngine engine;
    FastAccelStepper *stepper_DA = nullptr;
    FastAccelStepper *stepper_DEC = nullptr;
    volatile bool tracking = false;

    static int32_t degreesToSteps(double axisDegrees) {
        return (int32_t)lround(axisDegrees / 360.0 * STEPS_PER_AXIS_TURN);
    }
};

#endif // MOTORS_H
