#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>
#include <FastAccelStepper.h>
#include "config.h"
#include "debug.h"

/**
 * Couche mince au-dessus de FastAccelStepper, en pas moteur.
 * Les pas sont produits par un périphérique matériel de l'ESP32 :
 * aucune tâche ne boucle sur le processeur pendant un mouvement.
 * Seule la classe Mount appelle ces fonctions, depuis une seule tâche.
 */
class Motors {
public:
    bool initialize() {
        engine.init();

        da = engine.stepperConnectToPin(PIN_AD_STEP);
        dec = engine.stepperConnectToPin(PIN_DEC_STEP);
        if (!da || !dec) {
            dbg().println("Motors: stepperConnectToPin a échoué");
            da = dec = nullptr;
            return false;
        }

        da->setDirectionPin(PIN_AD_DIR, !AD_DIR_INVERT);
        dec->setDirectionPin(PIN_DEC_DIR, !DEC_DIR_INVERT);

        da->setAcceleration(MOTOR_ACCEL);
        dec->setAcceleration(MOTOR_ACCEL);
        return true;
    }

    bool ok() const { return da && dec; }

    /** Compteur de pas depuis le démarrage. */
    int32_t stepsDA() const { return da ? da->getCurrentPosition() : 0; }
    int32_t stepsDEC() const { return dec ? dec->getCurrentPosition() : 0; }

    /** Va au compteur de pas donné, à vitesse rapide. Marche aussi en plein mouvement. */
    void gotoStepsDA(int32_t target) {
        if (!da) return;
        da->setSpeedInHz(SLEW_SPEED_HZ);
        da->moveTo(target);
    }

    void gotoStepsDEC(int32_t target) {
        if (!dec) return;
        dec->setSpeedInHz(SLEW_SPEED_HZ);
        dec->moveTo(target);
    }

    /** Suivi sidéral : rotation continue de l'axe d'ascension droite. */
    void trackDA() {
        if (!da) return;
        if (da->setSpeedInUs(TRACKING_STEP_US) < 0) {
            dbg().println("Motors: vitesse de suivi refusée");
            return;
        }
        da->runForward();
    }

    /** Arrêt avec décélération. */
    void stopDA() { if (da) da->stopMove(); }
    void stopDEC() { if (dec) dec->stopMove(); }

    bool isRunningDA() const { return da && da->isRunning(); }
    bool isRunningDEC() const { return dec && dec->isRunning(); }

private:
    FastAccelStepperEngine engine;
    FastAccelStepper *da = nullptr;
    FastAccelStepper *dec = nullptr;
};

#endif // MOTORS_H
