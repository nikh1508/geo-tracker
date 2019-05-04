#include <Arduino.h>
#include <SoftwareSerial.h>
#include "SIM808.h"

SIM808 module(3, 2, 6);
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
        Serial.println("Failed to connect to GPRS");
    }
    // module.switchGPS(1);
    // delay(2000);
    // module.switchGPS(0);
    // module.getGPSCoordinates();
    bool resp = module.postHTTP(post_url);
    Serial.println((resp ? "SUCCESS" : "FAILED"));
}

void loop()
{
    module.serialToModule();
}
