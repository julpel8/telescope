#include <Arduino.h>
#include "config.h"
#include "telescope_state.h"
#include "wifitelescope.h"
#include "rts2.h"
#include "lx200.h"
#include "motors.h"

TelescopeState telescope;
RTS2 rts2;
LX200 lx200(Serial2);
Motors motors;

void taskTracking(void *params)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        motors.updateTracking();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello World");
    
    Serial.println("Creating AP");
    wifiTelescope.initAP();

    motors.initialize();

    xTaskCreate(
        taskTracking,   /* Task function. */
        "taskTracking", /* name of task. */
        4096,           /* Stack size of task */
        NULL,           /* parameter of the task */
        1,              /* priority of the task */
        NULL);          /* Task handle to keep track of created task */

    rts2.begin();
}

void loop()
{
    TelescopeState::Coords from, to;
    if (telescope.takeGoto(from, to))
    {
        Serial.printf("Goto: RA %+f, DEC %+f\n", from.ra - to.ra, from.dec - to.dec);
        motors.moveDA(from.ra - to.ra);
        motors.moveDEC(from.dec - to.dec);
    }
    delay(1000);
}
