#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * État du télescope, partagé entre le serveur web et loop().
 * Le serveur web tourne dans sa propre tâche : chaque accès passe par
 * un verrou pour qu'une tâche ne lise jamais une valeur à moitié écrite.
 */
class TelescopeState {
public:
    struct Coords {
        double ra;
        double dec;
    };

    /** Position actuelle, en degrés. */
    Coords position() {
        portENTER_CRITICAL(&lock);
        Coords c = current;
        portEXIT_CRITICAL(&lock);
        return c;
    }

    /** Demande un pointage vers ra/dec, en degrés. */
    void requestGoto(double ra, double dec) {
        portENTER_CRITICAL(&lock);
        target = {ra, dec};
        gotoPending = true;
        portEXIT_CRITICAL(&lock);
    }

    /**
     * Récupère un pointage en attente.
     * Renvoie false s'il n'y en a pas. Sinon remplit from et to,
     * et considère le télescope déjà arrivé à to.
     */
    bool takeGoto(Coords &from, Coords &to) {
        portENTER_CRITICAL(&lock);
        bool pending = gotoPending;
        if (pending) {
            from = current;
            to = target;
            current = target;
            gotoPending = false;
        }
        portEXIT_CRITICAL(&lock);
        return pending;
    }

    double longitude() const { return SITE_LONGITUDE; }
    double latitude() const { return SITE_LATITUDE; }
    double altitude() const { return SITE_ALTITUDE; }

private:
    portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
    Coords current = {0, 0};
    Coords target = {0, 0};
    bool gotoPending = false;
};

extern TelescopeState telescope;
