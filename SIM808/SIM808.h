/*
LED Status Codes
    connectGprs - 1
    switchGPS - 2
    checkGPSFix - 3
    getGPSCoordinates - 4
*/
#ifndef SIM808_H
#define SIM808_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#define sim (*port)
#define LED_DLY_TIME 200          //LED blink delay
#define SERIAL_TIMEOUT 500        //Serial Timeout in ms
#define TRIES 5                   // AT Command Retries
#define GPS_LOC_FIX_TIMEOUT 60000 //GPS Location Fix Timeout
#define HTTP_RESPONSE 15000       //HTTP Response Timeout
#define debug                     //Debug Mode

class SIM808
{
    int rx;
    int tx;
    int led_pin;
    String latitude; //No need to parse into float since the valuse will be posted as String in json
    String longitude;
    String sat_count;
    SoftwareSerial *port;

    // void blink(int count);
    // void flush();
    // void sendAT(String command);
    // bool sendAT(String command, String expected);
    // String readResponse();
    // bool readResponse(String expected);

  public:
    SIM808(int _rx, int _tx, int _led) : rx(_rx), tx(_tx), led_pin(_led)
    {
        port = new SoftwareSerial(rx, tx);
        sim.begin(9600);
        pinMode(led_pin, OUTPUT);
        latitude = "0000.000000";   //ddmm.mmmmmm   //HTTP Post returns error if the coordinates are zeroes
        longitude = "00000.000000"; //ddd.mmmmmm
        // latitude = "1249.404180";
        // longitude = "8002.539566";
        sat_count = "0";
    }
    //Serial
    void serialToModule();
    void blink(int count);
    void blink(int status, int command);
    void flush();
    void sendAT(String command);
    bool sendAT(String command, String expected);
    String readResponse();
    bool readResponse(String expected);
    //GPRS
    bool connectGprs();
    //GPS
    void switchGPS(bool state);
    bool checkGPSFix();
    void getGPSCoordinates();
    //HTTP
    bool postHTTP(const char *);
};

void SIM808::blink(int count)
{
    while ((count > 0) ? count-- : count) //blinks indefinitely for -ve
    {
        digitalWrite(led_pin, HIGH);
        delay(LED_DLY_TIME);
        digitalWrite(led_pin, LOW);
        delay(LED_DLY_TIME);
    }
}

void SIM808::blink(int status, int command)
{
    blink(status);
    delay(500);
    blink(command);
    delay(1000);
}

void SIM808::flush()
{
    while (sim.available())
        char ch = sim.read();
}

void SIM808::sendAT(String command)
{
#ifdef debug
    Serial.println("AT : " + command);
#endif
    sim.println(command);
}

bool SIM808::sendAT(String command, String expected)
{
    int tries = TRIES;
    bool resp = false;
    while (!resp && tries--)
    {
        if (!resp)
        {
            sendAT(command);
            flush();
            delay(100);
        }
        resp = readResponse(expected);
    }
    return resp;
}

String SIM808::readResponse()
{
    unsigned long start_time = millis();
    unsigned long last_read = 0;
    String response = "";
    digitalWrite(led_pin, HIGH);
    while ((millis() - start_time) < SERIAL_TIMEOUT || sim.available() > 0)
    {
        char ch;
        if (sim.available())
        {
            ch = sim.read();
            response += ch;
            last_read = millis();
        }
        if ((ch == '\n' || ch == '\r') && last_read != 0 && millis() - last_read > 20)
            break;
    }
    digitalWrite(led_pin, LOW);
    return response;
}

bool SIM808::readResponse(String expected)
{
    String response = readResponse();
#ifdef debug
    Serial.println("Response : " + response);
#endif
    if (response.indexOf(expected) != -1)
        return true;
    else
        return false;
}

void SIM808::serialToModule()
{
    if (Serial.available())
    {
        char ch = Serial.read();
        sim.print(ch);
    }
    if (sim.available())
    {
        char ch = sim.read();
        Serial.print(ch);
    }
}

bool SIM808::connectGprs()
{
    bool connected = true;
    String gprsCommands[][2] = {
        {"AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK"},
        {"AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"", "OK"},
        {"AT+SAPBR=0,1", "\n"},
        {"AT+SAPBR=1,1", "OK"},
        {"AT+SAPBR=2,1", "+SAPBR: 1,1"}};

    int commands = 5;
    for (int i = 0; i < commands; i++)
    {
        // int tries = TRIES;
        // bool resp = false;
        // while (!resp && tries--)
        // {
        //     if (!resp)
        //     {
        //         sendAT(gprsCommands[i][0]);
        //         flush();
        //         delay(100);
        //     }
        //     resp = readResponse(gprsCommands[i][1]);
        // }
        bool resp = sendAT(gprsCommands[i][0], gprsCommands[i][1]);
#ifdef debug
        // Serial.println(gprsCommands[i][0]);
        // Serial.println(resp);
        if (!resp)
        {
            Serial.println("FAILED!");
            // while (true)
            // {
            //     blink(1, i + 1);
            // }
        }
#endif
        if (!resp && i == 4) // The last AT Command returns the status of GPRS
            connected = false;
        delay(100);
    }
    return connected;
}

void SIM808::switchGPS(bool state)
{
    String command = String("AT+CGPSPWR=") + ((state) ? String("1") : String("0"));
    int tries = TRIES;
    // bool resp = false;
    // while (!resp && tries--)
    // {
    //     if (!resp)
    //     {
    //         sendAT(command);
    //         flush();
    //         delay(100);
    //     }
    //     resp = readResponse("OK");
    // }
    bool resp = sendAT(command, "OK");
#ifdef debug
    Serial.println("GPS Switching : " + String(resp));
    if (!resp)
    {
        Serial.println("FAILED!");
        while (true)
        {
            blink(2, 1);
        }
    }
#endif
}

bool SIM808::checkGPSFix()
{
    String command = String("AT+CGPSSTATUS?");
    int tries = TRIES;
    bool resp = false;
    String response;
    while (!resp && tries--)
    {
        if (!resp)
        {
            sendAT(command);
            flush();
            delay(100);
        }
        response = readResponse();
        resp = response.indexOf("Location") != -1;
    }
#ifdef debug
    Serial.println("Location Fix : " + response);
    if (!resp && tries == 0)
    {
        Serial.println("FAILED!");
        while (true)
        {
            blink(3, 1);
        }
    }
#endif

    if (response.indexOf("Unknown") != -1)
    {
        switchGPS(1);
        return false;
    }
    else if (response.indexOf("2D") != -1 || response.indexOf("3D") != -1)
        return true;
    else
        return false;
}

void SIM808::getGPSCoordinates()
{
    String command = "AT+CGPSINF=0";
    int tries = TRIES;
    bool resp = false;
    String response;
    while (!resp && tries--)
    {
        if (!resp)
        {
            sendAT(command);
            flush();
            delay(100);
        }
        response = readResponse();
        resp = response.indexOf("+CGPSINF") != -1;
    }
#ifdef debug
    Serial.println("Get Coordinates : " + response);
    if (!resp && tries == 0)
    {
        Serial.println("FAILED!");
        while (true)
        {
            blink(4, 1);
        }
    }
#endif
    if (resp)
    {
        int index[5];
        index[0] = response.indexOf(',');
        index[1] = response.indexOf(',', index[0] + 1);
        index[2] = response.indexOf(',', index[1] + 1);
        index[3] = response.lastIndexOf(','); //ignore last comma
        index[3] = response.lastIndexOf(',', index[3] - 1);
        index[4] = response.lastIndexOf(',', index[3] - 1);
        Serial.println(String(index[0]) + " " + String(index[1]) + " " + String(index[2]) + " " + String(index[3]) + " " + String(index[4]));
        longitude = response.substring(index[0] + 1, index[1]);
        latitude = response.substring(index[1] + 1, index[2]);
        sat_count = response.substring(index[4] + 1, index[3]);
    }
#ifdef debug
    Serial.println("Latitude : " + latitude);
    Serial.println("Longitude : " + longitude);
    Serial.println("Satellite Count : " + sat_count);
#endif
}

bool SIM808::postHTTP(const char *url)
{
    String post_data = "{\"lat\":" + latitude + ", \"lng\":" + longitude + ", \"sat_count\":" + sat_count + "}";
    // String post_data = "{\"lat\":0000.000000, \"lng\":00000.000000, \"sat_count\":0}";
    // String post_data = "{\"lat\":1249.404180, \"lng\":8002.539566, \"sat_count\":13}";
    String response;
    unsigned long sent;
    bool resp;

    String gprsCommands[] = {
        "AT+HTTPINIT",
        "AT+HTTPPARA=\"CID\",1",
        "AT+HTTPPARA=\"URL\",\"" + String(url) + "\"",
        "AT+HTTPPARA=\"CONTENT\",\"application/json\""};
    sendAT("AT+HTTPTERM", "\n"); // In case there is an open HTTP connection
    for (String command : gprsCommands)
    {
        sendAT(command, "OK");
        delay(100);
    }
    // Serial.println("AT : " + ("AT+HTTPDATA=" + String(post_data.length() + 5) + ",10000"));
    resp = sendAT(("AT+HTTPDATA=" + String(post_data.length() + 5) + ",5000"), "DOWNLOAD");
    // if (!resp)
    //     Serial.println("Not DOWNLOAD");
    // else
    //     Serial.println("Recvd DOWNLOAD");
    delay(100);
    sendAT(post_data);
    // Serial.println("Posted : " + post_data);
    sent = millis();
    while (!Serial.available() && millis() - sent < 5000)
        ;
    response = readResponse();
    Serial.println("Post Resp : " + response);
    if (response.indexOf("OK") != -1)
        Serial.println("Posted OK");
    else
        Serial.println("Posted Not OK");
    delay(1000);
    sendAT("AT+HTTPACTION=1", "OK");
    while (!Serial.available() && millis() - sent < HTTP_RESPONSE)
        ;
    resp = readResponse("200");
    if (!resp)
        Serial.println("Not 200");
    else
        Serial.println("200 OK");
    // Serial.println("AT : AT+HTTPREAD");
    sendAT("AT+HTTPREAD", "OK");
    delay(100);
    // Serial.println("AT : AT+HTTPTERM");
    sendAT("AT+HTTPTERM", "OK");
    return resp;
}
#endif