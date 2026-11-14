#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * Boîte aux lettres entre les protocoles (RTS2, LX200) et la classe Mount.
 *
 * Les protocoles lisent la position et déposent des demandes. Mount, seule à
 * piloter les moteurs, prend les demandes et publie la position. Chaque accès
 * passe par un verrou : plusieurs tâches s'en servent en même temps.
 */
class TelescopeState {
public:
    struct Coords {
        double ra;   // degrés, 0 à 360
        double dec;  // degrés, -90 à +90
    };

    struct Requests {
        bool stop = false;
        bool sync = false;
        Coords syncCoords = {0, 0};
        bool gotoRequested = false;
        Coords gotoTarget = {0, 0};
    };

    // --- Pour les protocoles ------------------------------------------------

    Coords position() {
        portENTER_CRITICAL(&lock);
        Coords c = current;
        portEXIT_CRITICAL(&lock);
        return c;
    }

    /** Vrai pendant un pointage ou un arrêt en cours. */
    bool isSlewing() {
        portENTER_CRITICAL(&lock);
        bool s = slewing;
        portEXIT_CRITICAL(&lock);
        return s;
    }

    void requestGoto(double ra, double dec) {
        portENTER_CRITICAL(&lock);
        pending.gotoRequested = true;
        pending.gotoTarget = {ra, dec};
        pending.stop = false;
        slewing = true;  // visible tout de suite, sans attendre Mount
        portEXIT_CRITICAL(&lock);
    }

    /** Déclare que le télescope vise ra/dec, sans bouger. */
    void requestSync(double ra, double dec) {
        portENTER_CRITICAL(&lock);
        pending.sync = true;
        pending.syncCoords = {ra, dec};
        pending.gotoRequested = false;
        current = {ra, dec};  // visible tout de suite, sans attendre Mount
        portEXIT_CRITICAL(&lock);
    }

    void requestStop() {
        portENTER_CRITICAL(&lock);
        pending.stop = true;
        pending.gotoRequested = false;
        portEXIT_CRITICAL(&lock);
    }

    double longitude() const { return SITE_LONGITUDE; }
    double latitude() const { return SITE_LATITUDE; }
    double altitude() const { return SITE_ALTITUDE; }

    // --- Pour Mount seulement -----------------------------------------------

    /** Lit les demandes en attente et les efface. */
    Requests takeRequests() {
        portENTER_CRITICAL(&lock);
        Requests r = pending;
        pending = Requests();
        portEXIT_CRITICAL(&lock);
        return r;
    }

    /**
     * Publie la position calculée par Mount. Une demande déposée depuis le
     * dernier takeRequests() garde la main : sa position ou son état
     * « en mouvement » restent visibles jusqu'au prochain passage de Mount.
     */
    void publish(Coords pos, bool isMoving) {
        portENTER_CRITICAL(&lock);
        if (!pending.sync) current = pos;
        slewing = isMoving || pending.gotoRequested;
        portEXIT_CRITICAL(&lock);
    }

private:
    portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
    Coords current = {START_RA, START_DEC};
    bool slewing = false;
    Requests pending;
};

extern TelescopeState telescope;
