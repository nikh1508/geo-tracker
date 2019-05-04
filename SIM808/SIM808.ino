#include <Arduino.h>
#include <SoftwareSerial.h>

#define debug
#include "SIM808.h"

SIM808 module(3, 2, 6); //RX | TX | LED
const char *post_url = "http://tracker.nikhilkr.com/api/post_location/666";

void setup()
{
    Serial.begin(9600);
    // module.blink(-1);
    Serial.println("Started");
    bool connected = module.connectGprs();
    while (!connected)
    {
        connected = module.connectGprs();
        Serial.println("Failed to connect to GPRS. Retrying.");
    }
    module.switchGPS(1);
    delay(2000);
    bool gps_fix = module.checkGPSFix();
    while (!gps_fix)
    {
        gps_fix = module.checkGPSFix();
        Serial.println("Retrying For GPS Fix");
        delay(500);
    }
    module.getGPSCoordinates();
    module.switchGPS(0);
    bool resp = module.postHTTP(post_url);
    Serial.println((resp ? "SUCCESS" : "FAILED"));
}

void loop()
{
    module.serialToModule();
}
