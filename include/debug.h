#pragma once

#include <Arduino.h>

// Sortie des messages de debug. Muette par défaut. setup() la branche sur
// Serial, sauf quand la prise USB sert au protocole LX200 : un message de
// debug casserait alors les réponses envoyées au logiciel.

class NullPrint : public Print {
public:
    size_t write(uint8_t) override { return 1; }
    size_t write(const uint8_t *, size_t size) override { return size; }
};

inline Print *&debugOutputPtr() {
    static NullPrint muted;
    static Print *out = &muted;
    return out;
}

inline Print &dbg() { return *debugOutputPtr(); }

inline void setDebugOutput(Print &out) { debugOutputPtr() = &out; }
