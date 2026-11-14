#pragma once

#include <Arduino.h>
#include <Preferences.h>

/** Port série utilisé par le protocole LX200. */
enum class Lx200Serial : uint8_t {
    Pins = 0,  // broches 16 (RX) / 17 (TX), debug sur la prise USB
    Usb = 1,   // prise USB de la carte, debug coupé
    Off = 2,   // pas de LX200 par câble
};

inline const char *lx200SerialName(Lx200Serial mode) {
    switch (mode) {
    case Lx200Serial::Usb: return "usb";
    case Lx200Serial::Off: return "off";
    default: return "pins";
    }
}

inline bool lx200SerialFromName(const String &name, Lx200Serial &mode) {
    if (name == "pins") mode = Lx200Serial::Pins;
    else if (name == "usb") mode = Lx200Serial::Usb;
    else if (name == "off") mode = Lx200Serial::Off;
    else return false;
    return true;
}

/**
 * Réglages modifiables depuis la page web, gardés en mémoire flash.
 * Un changement prend effet au redémarrage suivant.
 */
class Settings {
public:
    void load() {
        Preferences prefs;
        // Au tout premier démarrage, l'espace n'existe pas encore : on garde
        // les valeurs par défaut.
        if (!prefs.begin(NAMESPACE, true)) return;
        uint8_t v = prefs.getUChar(KEY_LX200_SERIAL, uint8_t(Lx200Serial::Pins));
        if (v <= uint8_t(Lx200Serial::Off)) lx200 = Lx200Serial(v);
        prefs.end();
    }

    bool saveLx200Serial(Lx200Serial mode) {
        Preferences prefs;
        if (!prefs.begin(NAMESPACE, false)) return false;
        bool ok = prefs.putUChar(KEY_LX200_SERIAL, uint8_t(mode)) == 1;
        prefs.end();
        if (ok) lx200 = mode;
        return ok;
    }

    Lx200Serial lx200Serial() const { return lx200; }

    /**
     * Demande un redémarrage dans une demi-seconde, le temps que la réponse
     * HTTP parte. C'est loop() qui redémarre.
     */
    void requestReboot() {
        rebootRequestMs = millis();
        rebootRequested = true;
    }

    bool rebootDue() const {
        return rebootRequested && millis() - rebootRequestMs > 500;
    }

private:
    static constexpr const char *NAMESPACE = "telescope";
    static constexpr const char *KEY_LX200_SERIAL = "lx200ser";

    Lx200Serial lx200 = Lx200Serial::Pins;
    volatile uint32_t rebootRequestMs = 0;
    volatile bool rebootRequested = false;
};

extern Settings settings;
