#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "debug.h"
#include "settings.h"
#include "telescope_state.h"
#include "motors.h"
#include "mount.h"
#include "wifitelescope.h"
#include "rts2.h"
#include "lx200.h"

Settings settings;
TelescopeState telescope;
Motors motors;
Mount mount(motors, telescope);
RTS2 rts2;

// LX200 par câble : Serial2 (broches), Serial (USB) ou rien.
Stream *lx200Port = nullptr;
Lx200Session serialSession;

// LX200 par Wi-Fi.
WiFiServer lx200Server(LX200_TCP_PORT);
WiFiClient lx200Clients[LX200_TCP_MAX_CLIENTS];
Lx200Session tcpSessions[LX200_TCP_MAX_CLIENTS];
uint32_t lastActivityMs[LX200_TCP_MAX_CLIENTS];

void taskMount(void *params)
{
    TickType_t lastWake = xTaskGetTickCount();
    while (1)
    {
        mount.update();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(50));
    }
}

void setupSerialPorts()
{
    switch (settings.lx200Serial())
    {
    case Lx200Serial::Usb:
        // La prise USB parle LX200 : aucun message de debug, ils casseraient
        // les réponses. La sortie de debug reste muette.
        Serial.begin(LX200_SERIAL_BAUD);
        lx200Port = &Serial;
        break;
    case Lx200Serial::Pins:
        Serial.begin(DEBUG_BAUD);
        setDebugOutput(Serial);
        Serial2.begin(LX200_SERIAL_BAUD, SERIAL_8N1, PIN_LX200_RX, PIN_LX200_TX);
        lx200Port = &Serial2;
        break;
    case Lx200Serial::Off:
        Serial.begin(DEBUG_BAUD);
        setDebugOutput(Serial);
        break;
    }
    dbg().printf("LX200 série : %s\n", lx200SerialName(settings.lx200Serial()));
}

void acceptLx200Clients()
{
    WiFiClient client = lx200Server.accept();
    if (!client)
        return;

    // Place libre, sinon celle du client inactif depuis le plus longtemps.
    int slot = -1;
    for (int i = 0; i < LX200_TCP_MAX_CLIENTS && slot < 0; i++)
        if (!lx200Clients[i].connected())
            slot = i;
    if (slot < 0)
    {
        slot = 0;
        for (int i = 1; i < LX200_TCP_MAX_CLIENTS; i++)
            if (millis() - lastActivityMs[i] > millis() - lastActivityMs[slot])
                slot = i;
        lx200Clients[slot].stop();
    }

    client.setNoDelay(true);
    lx200Clients[slot] = client;
    tcpSessions[slot].reset();
    lastActivityMs[slot] = millis();
    dbg().printf("LX200 Wi-Fi : client %s en place %d\n",
                 client.remoteIP().toString().c_str(), slot);
}

void pollLx200()
{
    if (lx200Port)
        serialSession.poll(*lx200Port);

    acceptLx200Clients();
    for (int i = 0; i < LX200_TCP_MAX_CLIENTS; i++)
    {
        if (!lx200Clients[i].connected())
            continue;
        if (lx200Clients[i].available())
            lastActivityMs[i] = millis();
        tcpSessions[i].poll(lx200Clients[i]);
    }
}

void setup()
{
    settings.load();
    setupSerialPorts();

    wifiTelescope.initAP();

    if (motors.initialize())
        mount.begin();

    xTaskCreate(taskMount, "mount", 4096, NULL, 2, NULL);

    rts2.begin();
    lx200Server.begin();
    dbg().printf("LX200 Wi-Fi : port %u\n", LX200_TCP_PORT);
}

void loop()
{
    ArduinoOTA.handle();
    pollLx200();
    if (settings.rebootDue())
        ESP.restart();
    delay(5);
}
