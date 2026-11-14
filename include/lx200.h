#ifndef LX200_H
#define LX200_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "lx200_coords.h"
#include "telescope_state.h"

/**
 * Une session LX200 par liaison (câble série ou connexion Wi-Fi).
 *
 * Une commande LX200 commence par ':' et finit par '#', par exemple
 * ":GR#" (lire l'ascension droite). La réponse part sur le même flux.
 * Seul l'octet 0x06 (demande du type de monture) arrive sans ':' ni '#'.
 *
 * La session ne touche jamais aux moteurs : elle passe par TelescopeState.
 */
class Lx200Session {
public:
    void reset() {
        length = 0;
        inCommand = false;
        longFormat = true;
    }

    /** Traite tous les octets reçus sur io. À appeler souvent. */
    void poll(Stream &io) {
        while (io.available()) {
            char c = io.read();
            if (!inCommand) {
                if (c == 0x06) {
                    io.write('P');  // monture équatoriale
                } else if (c == ':') {
                    inCommand = true;
                    length = 0;
                }
                // Le reste est ignoré, par exemple le '#' que Stellarium
                // envoie avant certaines commandes pour vider la ligne.
                continue;
            }
            if (c == '#') {
                buffer[length] = '\0';
                inCommand = false;
                handle(io, buffer);
            } else if (c == ':') {
                length = 0;  // nouvelle commande sans '#' avant
            } else if (length < sizeof(buffer) - 1) {
                buffer[length++] = c;
            } else {
                inCommand = false;  // commande trop longue : jetée
            }
        }
    }

private:
    char buffer[32];
    size_t length = 0;
    bool inCommand = false;
    bool longFormat = true;

    // Cible donnée par :Sr et :Sd, utilisée par :MS (pointage) et :CM (synchro).
    double targetRaHours = 0;
    double targetDecDeg = 0;

    static void reply(Stream &io, const char *text) {
        io.write(reinterpret_cast<const uint8_t *>(text), strlen(text));
    }

    void handle(Stream &io, const char *cmd) {
        char out[32];

        if (strcmp(cmd, "GR") == 0) {
            lx200::formatRA(telescope.position().ra / 15.0, longFormat, out, sizeof(out) - 1);
            strcat(out, "#");
            reply(io, out);
        } else if (strcmp(cmd, "GD") == 0) {
            lx200::formatDec(telescope.position().dec, longFormat, out, sizeof(out) - 1);
            strcat(out, "#");
            reply(io, out);
        } else if (strcmp(cmd, "U") == 0) {
            longFormat = !longFormat;
        } else if (strncmp(cmd, "Sr", 2) == 0) {
            reply(io, lx200::parseRA(cmd + 2, targetRaHours) ? "1" : "0");
        } else if (strncmp(cmd, "Sd", 2) == 0) {
            reply(io, lx200::parseDec(cmd + 2, targetDecDeg) ? "1" : "0");
        } else if (strcmp(cmd, "MS") == 0) {
            telescope.requestGoto(targetRaHours * 15.0, targetDecDeg);
            reply(io, "0");
        } else if (strcmp(cmd, "CM") == 0) {
            telescope.requestSync(targetRaHours * 15.0, targetDecDeg);
            reply(io, "Coordinates matched.#");
        } else if (cmd[0] == 'Q') {
            // :Q# arrête tout. :Qn# :Qs# :Qe# :Qw# arrêtent un sens de
            // déplacement manuel, non géré : on arrête tout aussi.
            telescope.requestStop();
        } else if (strcmp(cmd, "D") == 0) {
            reply(io, telescope.isSlewing() ? "\x7F#" : "#");
        } else if (strcmp(cmd, "GVP") == 0) {
            reply(io, "ESP32 Telescope#");
        } else if (strcmp(cmd, "GVN") == 0) {
            reply(io, "1.0#");
        } else if (strcmp(cmd, "GM") == 0) {
            reply(io, "Site 1#");
        } else if (strcmp(cmd, "Gt") == 0) {
            lx200::formatLatitude(telescope.latitude(), out, sizeof(out) - 1);
            strcat(out, "#");
            reply(io, out);
        } else if (strcmp(cmd, "Gg") == 0) {
            lx200::formatLongitude(telescope.longitude(), out, sizeof(out) - 1);
            strcat(out, "#");
            reply(io, out);
        } else {
            dbg().printf("LX200: commande inconnue :%s#\n", cmd);
        }
    }
};

#endif // LX200_H
