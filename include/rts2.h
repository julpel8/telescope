#ifndef RTS2_H
#define RTS2_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "ArduinoJson.h"
#include "config.h"
#include "html_page.h"
#include "settings.h"
#include "telescope_state.h"

class RTS2 {
public:
    /**
     * @brief Constructeur : initialise le serveur web.
     */
    RTS2() : server(80) {}

    /**
     * @brief Initialise le serveur web et configure les routes.
     */
    void begin() {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/html", INDEX_HTML);
        });

        server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"ret\": 0}");
            settings.requestReboot();
        });

        // Réglages de la page web. Lecture en GET, écriture en POST :
        // la carte enregistre puis redémarre.
        server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            String json = String("{\"lx200_serial\":\"") + lx200SerialName(settings.lx200Serial())
                          + "\",\"lx200_tcp_port\":" + LX200_TCP_PORT + "}";
            request->send(200, "application/json", json);
        });

        server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request) {
            Lx200Serial mode;
            if (!request->hasParam("lx200_serial", true)
                || !lx200SerialFromName(request->getParam("lx200_serial", true)->value(), mode)) {
                request->send(400, "application/json", "{\"ret\": 1}");
                return;
            }
            if (!settings.saveLx200Serial(mode)) {
                request->send(500, "application/json", "{\"ret\": 2}");
                return;
            }
            request->send(200, "application/json", "{\"ret\": 0}");
            settings.requestReboot();
        });

        server.on("/api/devbytype", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (request->hasParam("t") && request->getParam("t")->value() == "2") {
                request->send(200, "application/json", "[\"telescope\"]");
            } else {
                request->send(404, "application/json", "{\"message\":\"Not found\"}");
            }
        });

        server.on("/api/deviceinfo", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (request->hasParam("d") && request->getParam("d")->value() == "telescope") {
                request->send(200, "application/json", "{\"readonly\": 0}");
            } else {
                request->send(404, "application/json", "{\"message\":\"Not found\"}");
            }
        });

        server.on("/api/get", HTTP_GET, [](AsyncWebServerRequest *request) {
            const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5);
            DynamicJsonDocument doc(capacity);
            JsonObject d = doc.createNestedObject("d");
            d["LONGITUD"] = telescope.longitude();
            d["LATITUDE"] = telescope.latitude();
            d["ALTITUDE"] = telescope.altitude();
            d["target_distance"] = 0;

            JsonObject d_TEL = d.createNestedObject("TEL");
            TelescopeState::Coords pos = telescope.position();
            d_TEL["ra"] = pos.ra;
            d_TEL["dec"] = pos.dec;

            String json;
            serializeJson(doc, json);
            if (request->hasParam("d") && request->getParam("d")->value() == "telescope") {
                request->send(200, "application/json", json);
            } else {
                request->send(404, "application/json", "{\"message\":\"Not found\"}");
            }
        });

        server.on("/api/cmd", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("d") || request->getParam("d")->value() != "telescope") {
                request->send(404, "application/json", "{\"message\":\"Not found\"}");
            } else if (!request->hasParam("c")) {
                request->send(400, "application/json", "{\"ret\": 1}");
            } else {
                String moveCmd = request->getParam("c")->value();
                int r = moveCmd.indexOf(" ");
                int d = moveCmd.indexOf(" ", r + 1);
                double ra = moveCmd.substring(r + 1, d).toDouble();
                double dec = moveCmd.substring(d + 1).toDouble();
                telescope.requestGoto(ra, dec);
                request->send(200, "application/json", "{\"ret\": 0}");
            }
        });

        server.onNotFound(notFound);
        server.begin();
    }

private:
    AsyncWebServer server;

    /**
     * @brief Gère les requêtes non trouvées.
     * @param request Pointeur vers la requête.
     */
    static void notFound(AsyncWebServerRequest *request) {
        request->send(404, "application/json", "{\"message\":\"Not found\"}");
    }
};

#endif // RTS2_H