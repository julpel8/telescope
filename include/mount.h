#pragma once

#include <Arduino.h>
#include <math.h>
#include "esp_timer.h"
#include "config.h"
#include "debug.h"
#include "motors.h"
#include "telescope_state.h"

/**
 * Pilote la monture : pointage, suivi, arrêt, synchronisation.
 *
 * La position vient toujours des compteurs de pas des moteurs, plus la
 * rotation du ciel depuis la dernière synchronisation. Un arrêt en route
 * laisse donc une position juste.
 *
 * update() doit être appelée régulièrement par une seule tâche.
 */
class Mount {
public:
    using Coords = TelescopeState::Coords;

    Mount(Motors &m, TelescopeState &s) : motors(m), state(s) {}

    void begin() {
        setReference({START_RA, START_DEC});
        state.publish(currentPosition(), false);
    }

    void update() {
        if (!motors.ok()) return;

        TelescopeState::Requests req = state.takeRequests();
        if (req.stop) {
            motors.stopDA();
            motors.stopDEC();
            phase = Phase::Stopping;
            dbg().println("Mount: arrêt");
        }
        if (req.sync) {
            setReference(req.syncCoords);
            dbg().printf("Mount: synchro RA %.4f DEC %.4f\n", req.syncCoords.ra, req.syncCoords.dec);
        }
        if (req.gotoRequested) startGoto(req.gotoTarget);

        bool stopped = !motors.isRunningDA() && !motors.isRunningDEC();
        switch (phase) {
        case Phase::Tracking:
            if (!motors.isRunningDA()) motors.trackDA();
            break;
        case Phase::Slewing:
            if (stopped) {
                if (passes < GOTO_MAX_PASSES && moveTowardTarget()) {
                    passes++;
                } else {
                    phase = Phase::Tracking;
                    motors.trackDA();
                    dbg().printf("Mount: arrivé en %d passe(s)\n", passes);
                }
            }
            break;
        case Phase::Stopping:
            if (stopped) {
                phase = Phase::Tracking;
                motors.trackDA();
            }
            break;
        }

        state.publish(currentPosition(), phase != Phase::Tracking);
    }

private:
    enum class Phase { Tracking, Slewing, Stopping };

    Motors &motors;
    TelescopeState &state;

    Phase phase = Phase::Tracking;
    Coords target = {0, 0};
    int passes = 0;

    // Dernière synchronisation : à l'instant refTimeUs, avec ces compteurs
    // de pas, le télescope visait ref.
    Coords ref = {START_RA, START_DEC};
    int32_t refStepsDA = 0;
    int32_t refStepsDEC = 0;
    int64_t refTimeUs = 0;

    static double norm360(double deg) {
        deg = fmod(deg, 360.0);
        return deg < 0 ? deg + 360.0 : deg;
    }

    static double wrap180(double deg) {
        deg = norm360(deg);
        return deg > 180.0 ? deg - 360.0 : deg;
    }

    static int32_t absSteps(int32_t s) { return s < 0 ? -s : s; }

    void setReference(Coords c) {
        ref = c;
        refStepsDA = motors.stepsDA();
        refStepsDEC = motors.stepsDEC();
        refTimeUs = esp_timer_get_time();
    }

    Coords currentPosition() const {
        double elapsedS = (esp_timer_get_time() - refTimeUs) / 1e6;
        // Pas positifs en ascension droite = sens du suivi : l'ascension
        // droite visée baisse. Le ciel qui tourne la fait monter.
        double ra = ref.ra - (motors.stepsDA() - refStepsDA) * DEG_PER_STEP
                    + elapsedS * SIDEREAL_DEG_PER_S;
        double dec = ref.dec + (motors.stepsDEC() - refStepsDEC) * DEG_PER_STEP;
        return {norm360(ra), dec};
    }

    void startGoto(Coords t) {
        target.ra = norm360(t.ra);
        target.dec = constrain(t.dec, -90.0, 90.0);
        dbg().printf("Mount: pointage RA %.4f DEC %.4f\n", target.ra, target.dec);
        if (moveTowardTarget()) {
            phase = Phase::Slewing;
            passes = 1;
        } else {
            // Déjà sur la cible : le suivi continue.
            phase = Phase::Tracking;
        }
    }

    /**
     * Lance un mouvement vers la cible. Renvoie false si l'écart est déjà
     * sous la tolérance, sans rien lancer.
     */
    bool moveTowardTarget() {
        Coords cur = currentPosition();
        // Chemin court en ascension droite. Rien n'empêche encore le tube
        // de passer sous l'horizon ou de toucher le pied.
        int32_t dRa = -lround(wrap180(target.ra - cur.ra) / DEG_PER_STEP);
        int32_t dDec = lround((target.dec - cur.dec) / DEG_PER_STEP);
        if (absSteps(dRa) <= GOTO_TOLERANCE_STEPS && absSteps(dDec) <= GOTO_TOLERANCE_STEPS)
            return false;
        motors.gotoStepsDA(motors.stepsDA() + dRa);
        motors.gotoStepsDEC(motors.stepsDEC() + dDec);
        return true;
    }
};
