#include "Arduino.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <IRremote.hpp>

int httpServerPortNumber = 1234;
AsyncWebServer httpServer(httpServerPortNumber);

//String ssidWiFi = "ShawFromGhauri";
//String passwordWiFi = "bruhmomento";

String ssidWiFi = "ShawFromGhauri";
String passwordWiFi = "bruhmomento";

int pinTransmitterIR = 22;
int pinReceiverIR = 23;

bool transmitNewIRDataFromWebApp = false;

struct DataIRStruct
{
    String protocolIR = "";
    uint16_t addressIR = 0;
    uint8_t commandIR = 0;
};

struct DataIRStruct dataIRFromWebApp; 
struct DataIRStruct dataIRFromMCU; 

void sendIRData();
void receiveIRData();

void setup()
{
    Serial.begin(9600);

    SPIFFS.begin(true);

    IrSender.begin(pinTransmitterIR, DISABLE_LED_FEEDBACK);
    IrReceiver.begin(pinReceiverIR, DISABLE_LED_FEEDBACK);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidWiFi.c_str(), passwordWiFi.c_str());

    if (WiFi.waitForConnectResult() != WL_CONNECTED) 
    {
        Serial.println("WiFi Connection Failed!");
        while(1)
        {
            Serial.print(".");
            delay(1000);
        }
    }

    Serial.println();
    Serial.println("WiFi Connection Successful!");
    Serial.print("ESP32 Server's IP Address is: http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.print(httpServerPortNumber);
    Serial.print("/");
    Serial.println();

    httpServer.on("/api/get-all-configured-remotes", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println();
        Serial.println("/api/get-all-configured-remotes");

        File configuredRemotesDataFile = SPIFFS.open("/configured_remotes.json", "r");

        if (configuredRemotesDataFile == false) 
        {
            Serial.println("Failed to open file for reading!");
        }
        else
        {
            Serial.println("File opened for reading!");
            char configuredRemotesDataJSON_String[5120];
            DynamicJsonDocument configuredRemotesDataJSON_Object(5120);
            deserializeJson(configuredRemotesDataJSON_Object, configuredRemotesDataFile);
            serializeJson(configuredRemotesDataJSON_Object, configuredRemotesDataJSON_String);
            configuredRemotesDataFile.close();
            Serial.println("File closed after reading!");

            request->send(200, "application/json", configuredRemotesDataJSON_String);
        }
    });

    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/new-remote-configuration-data-from-web-app", [](AsyncWebServerRequest* request, JsonVariant& json) 
    {
        Serial.println();
        Serial.println("/POST/api/new-remote-configuration-data-from-web-app");
        
        if (not json.is<JsonObject>()) 
        {
            request->send(400, "text/plain", "Request is INVALID");
            Serial.println("Request is INVALID");
            return;
        }

        char responseJSONString[64];
        DynamicJsonDocument responseJSON(64);
        
        JsonObject newRemoteConfigurationDataFromWebAppJSON_Object = json.as<JsonObject>();
        Serial.println();
        Serial.println("New Remote Configuration: ");
        serializeJson(newRemoteConfigurationDataFromWebAppJSON_Object, Serial);
        Serial.println();
        Serial.println();

        DynamicJsonDocument configuredRemotesDataJSON_Object(8192);

        File configuredRemotesDataFile = SPIFFS.open("/configured_remotes.json", "r");

        if (configuredRemotesDataFile == false) 
        {
            Serial.println("Failed to open file for reading!");
            responseJSON["requestStatus"] = "Failed";
        }
        else
        {
            Serial.println("File opened for reading!");
            deserializeJson(configuredRemotesDataJSON_Object, configuredRemotesDataFile);
            configuredRemotesDataFile.close();
            Serial.println("File closed after reading!");

            JsonObject newRemoteConfigurationDataJSON_Object = configuredRemotesDataJSON_Object.createNestedObject();
            newRemoteConfigurationDataJSON_Object.set(newRemoteConfigurationDataFromWebAppJSON_Object);// = newRemoteConfigurationDataFromWebAppJSON_Object;
            //JsonObject newRemoteConfigurationDataJSON_Object = json.as<JsonObject>();

            File configuredRemotesDataFile = SPIFFS.open("/configured_remotes.json", "w");

            if (configuredRemotesDataFile == false) 
            {
                Serial.println("Failed to open file for writing!");
                responseJSON["requestStatus"] = "Failed";
            }
            else
            {
                Serial.println("File opened for writing!");
                serializeJson(configuredRemotesDataJSON_Object, configuredRemotesDataFile);
                //Serial.println();
                //serializeJson(configuredRemotesDataJSON_Object, Serial);
                //Serial.println();
                configuredRemotesDataFile.close();
                Serial.println("File closed after writing!");

                responseJSON["requestStatus"] = "Success";
            }
        }

        serializeJson(responseJSON, responseJSONString);
        request->send(200, "application/json", responseJSONString);
    }));

    httpServer.on("/api/ir-data-from-mcu", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println();
        Serial.println("/GET/api/ir-data-from-mcu");

        char dataIRFromMCUJSON_String[128];
        DynamicJsonDocument dataIRFromMCUJSON_Object(128);

        dataIRFromMCUJSON_Object["protocolIR"] = dataIRFromMCU.protocolIR;
        dataIRFromMCUJSON_Object["receiverAddress"] = String(dataIRFromMCU.addressIR, DEC);
        dataIRFromMCUJSON_Object["receiverCommand"] = String(dataIRFromMCU.commandIR, DEC);

        dataIRFromMCU.protocolIR = "";
        dataIRFromMCU.addressIR = 0;
        dataIRFromMCU.commandIR = 0;

        serializeJson(dataIRFromMCUJSON_Object, dataIRFromMCUJSON_String);

        request->send(200, "application/json", dataIRFromMCUJSON_String);
    });

    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ir-data-from-web-app", [](AsyncWebServerRequest* request, JsonVariant& json) 
    {
        Serial.println();
        Serial.println("/POST/api/ir-data-from-web-app");
        
        if (not json.is<JsonObject>()) 
        {
            request->send(400, "text/plain", "Request is INVALID");
            Serial.println("Request is INVALID");
            return;
        }

        JsonObject dataIRFromWebAppJSON_Object = json.as<JsonObject>();
        serializeJson(dataIRFromWebAppJSON_Object, Serial);
        Serial.println();

        transmitNewIRDataFromWebApp = true;

        dataIRFromWebApp.protocolIR = dataIRFromWebAppJSON_Object["protocolIR"].as<String>();
        dataIRFromWebApp.addressIR = uint16_t(dataIRFromWebAppJSON_Object["receiverAddress"]);
        dataIRFromWebApp.commandIR = uint8_t(dataIRFromWebAppJSON_Object["receiverCommand"]);

        Serial.print("Protocol: " + dataIRFromWebApp.protocolIR);
        Serial.print(" --- Address: ");
        Serial.print(dataIRFromWebApp.addressIR);
        Serial.print(" --- Command: ");
        Serial.print(dataIRFromWebApp.commandIR);
        Serial.println();

        char responseJSONString[64];
        DynamicJsonDocument responseJSON(64);
        responseJSON["requestStatus"] = "Success";
        serializeJson(responseJSON, responseJSONString);

        request->send(200, "application/json", responseJSONString);
    }));

    httpServer.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("/GET/favicon.ico");

        request->send(204, "text/plain", "Requested page not found - URC ESP32 Server");
        Serial.println("Did Not Share Favicon!");
    });

    httpServer.onNotFound([](AsyncWebServerRequest *request) 
    {
        request->send(404, "text/plain", "Requested page not found - URC ESP32 Server");
    });

    httpServer.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    httpServer.begin();
}

void loop()
{
    sendIRData();
    receiveIRData();
}

void sendIRData()
{
    if (transmitNewIRDataFromWebApp == true)
    {
        transmitNewIRDataFromWebApp = false;
        //IrReceiver.stop();

        if (dataIRFromWebApp.protocolIR == "NEC")
        {
            IrSender.sendNEC(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using NEC Protocol!");
        }
        else if (dataIRFromWebApp.protocolIR == "Samsung")
        {
            IrSender.sendSamsung(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using Samsung Protocol!");
        }
        else if (dataIRFromWebApp.protocolIR == "Sony")
        {
            IrSender.sendSony(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using Sony Protocol!");
        }
        else if (dataIRFromWebApp.protocolIR == "Panasonic")
        {
            IrSender.sendPanasonic(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using Panasonic Protocol!");
        }
        else if (dataIRFromWebApp.protocolIR == "LG")
        {
            IrSender.sendLG(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using LG Protocol!");
        }
        else if (dataIRFromWebApp.protocolIR == "JVC")
        {
            IrSender.sendJVC(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using JVC Protocol!");
        }
        else if (dataIRFromWebApp.protocolIR == "Sharp")
        {
            IrSender.sendSharp(dataIRFromWebApp.addressIR, dataIRFromWebApp.commandIR, 1);
            Serial.println("IR Command Transmitted using Sharp Protocol!");
        }
        else
        {
            Serial.println();
            Serial.println("The IR protocol didn't match any supported protocols!");
            Serial.println();
        }

        dataIRFromWebApp.protocolIR = "";
        dataIRFromWebApp.addressIR = 0;
        dataIRFromWebApp.commandIR = 0;

        //IrReceiver.restartAfterSend();
    }
}

void receiveIRData()
{
    if (IrReceiver.decode() == true)
    {
        if (IrReceiver.decodedIRData.numberOfBits > 8) // Filtering out garbage values due to random noise
        {
            Serial.println();
            IrReceiver.printIRResultShort(&Serial);
            //IrReceiver.printIRSendUsage(&Serial);

            dataIRFromMCU.protocolIR = IrReceiver.getProtocolString();
            dataIRFromMCU.addressIR = IrReceiver.decodedIRData.address;
            dataIRFromMCU.commandIR = IrReceiver.decodedIRData.command;

            Serial.print("Length: ");
            Serial.print(IrReceiver.decodedIRData.numberOfBits);
            Serial.print(" Bits");
            Serial.println();
            Serial.print("Scanned IR Protocol: " + dataIRFromMCU.protocolIR);
            Serial.print(" --- Address: " + String(dataIRFromMCU.addressIR, DEC));
            Serial.print(" --- Command: " + String(dataIRFromMCU.commandIR, DEC));
            Serial.println();
            Serial.println();
        }

        IrReceiver.resume();
    }
}
