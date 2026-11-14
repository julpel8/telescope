#pragma once

// Lecture et écriture des coordonnées au format LX200.
// Fonctions pures, sans Arduino : testées sur le PC (test/test_lx200_coords).

#include <math.h>
#include <stddef.h>
#include <stdio.h>

namespace lx200 {

namespace detail {

inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

/** Lit « 12 » ou « 12.34 ». Avance p. Faux s'il n'y a pas de chiffre. */
inline bool readNumber(const char *&p, double &value) {
    if (!isDigit(*p)) return false;
    double v = 0;
    while (isDigit(*p)) v = v * 10 + (*p++ - '0');
    if (*p == '.') {
        p++;
        double scale = 0.1;
        while (isDigit(*p)) {
            v += (*p++ - '0') * scale;
            scale /= 10;
        }
    }
    value = v;
    return true;
}

inline double wrap(double v, double period) {
    v = fmod(v, period);
    return v < 0 ? v + period : v;
}

}  // namespace detail

/**
 * Lit une valeur sexagésimale : signe facultatif, puis 2 ou 3 nombres
 * séparés par n'importe quel caractère qui n'est ni chiffre ni signe.
 * Exemples : "05:35:17", "05:35.3", " 05:35:17.25", "+22*01", "-00*30:00",
 * "+22\xDF" "01:30" (Stellarium met l'octet 0xDF comme symbole degré).
 */
inline bool parseSexagesimal(const char *s, double &value) {
    using namespace detail;
    const char *p = s;
    while (*p == ' ') p++;
    bool negative = false;
    if (*p == '+' || *p == '-') negative = (*p++ == '-');

    double parts[3] = {0, 0, 0};
    int count = 0;
    while (count < 3) {
        if (!readNumber(p, parts[count])) return false;
        count++;
        if (*p == '\0') break;
        while (*p && !isDigit(*p) && *p != '+' && *p != '-') p++;
        if (*p == '\0') break;
        if (!isDigit(*p)) return false;  // signe au milieu
    }

    if (count < 2 || parts[1] >= 60 || parts[2] >= 60) return false;
    double v = parts[0] + parts[1] / 60.0 + parts[2] / 3600.0;
    value = negative ? -v : v;
    return true;
}

/** Ascension droite, en heures : 0 ≤ h < 24. */
inline bool parseRA(const char *s, double &hours) {
    double v;
    if (!parseSexagesimal(s, v) || v < 0 || v >= 24) return false;
    hours = v;
    return true;
}

/** Déclinaison, en degrés : -90 à +90. */
inline bool parseDec(const char *s, double &degrees) {
    double v;
    if (!parseSexagesimal(s, v) || v < -90 || v > 90) return false;
    degrees = v;
    return true;
}

/** "HH:MM:SS" (long) ou "HH:MM.T" (court), sans le '#' final. */
inline void formatRA(double hours, bool longFormat, char *out, size_t size) {
    double h = detail::wrap(hours, 24.0);
    if (longFormat) {
        long t = lround(h * 3600.0) % 86400;
        snprintf(out, size, "%02ld:%02ld:%02ld", t / 3600, (t / 60) % 60, t % 60);
    } else {
        long t = lround(h * 600.0) % 14400;  // dixièmes de minute
        snprintf(out, size, "%02ld:%02ld.%01ld", t / 600, (t / 10) % 60, t % 10);
    }
}

/** "sDD<0xDF>MM'SS" (long) ou "sDD<0xDF>MM" (court), sans le '#' final. */
inline void formatDec(double degrees, bool longFormat, char *out, size_t size) {
    double d = fabs(degrees);
    if (d > 90) d = 90;
    if (longFormat) {
        long t = lround(d * 3600.0);
        char sign = (degrees < 0 && t > 0) ? '-' : '+';
        snprintf(out, size, "%c%02ld\xDF%02ld'%02ld", sign, t / 3600, (t / 60) % 60, t % 60);
    } else {
        long t = lround(d * 60.0);
        char sign = (degrees < 0 && t > 0) ? '-' : '+';
        snprintf(out, size, "%c%02ld\xDF%02ld", sign, t / 60, t % 60);
    }
}

/** Latitude du lieu : "sDD<0xDF>MM", nord positif. */
inline void formatLatitude(double degrees, char *out, size_t size) {
    formatDec(degrees, false, out, size);
}

/**
 * Longitude du lieu : "sDDD<0xDF>MM". Règle LX200 : ouest positif, donc le
 * signe est l'inverse de la convention habituelle (est positif).
 */
inline void formatLongitude(double eastDegrees, char *out, size_t size) {
    double west = -detail::wrap(eastDegrees + 180.0, 360.0) + 180.0;
    long t = lround(fabs(west) * 60.0);
    char sign = (west < 0 && t > 0) ? '-' : '+';
    snprintf(out, size, "%c%03ld\xDF%02ld", sign, t / 60, t % 60);
}

}  // namespace lx200
