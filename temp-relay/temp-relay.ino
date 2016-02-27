#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "EEPROMAnithing.h"
#include "Math.h"

// CONSTANTS
#define TEMP_ADJUSTMENT "tempAdjustment"
#define TEMP_MIN "tempMin"
#define TEMP_MAX "tempMax"
#define RUN_MODE "runMode"

// WIFI DECLARATION
const char* ssid = "Munis Red";
const char* pass = "32144584";
ESP8266WebServer server(80);
String errors = "";

// SYSTEM VARIABLES
float temp;
float tempAdjustment;
float tempMin;
float tempMax;
int runMode;

// SENSOR DECLARATION
#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// RELAY DECLARATION
int relay = 0;
boolean relayON = false;

// EEPROM
#define tempMinAddr 0
#define tempMaxAddr 10
#define tempAdjustmentAddr 20
#define runModeAddr 30
#define ssidStartAddr 100
#define ssidEndAddr 132
#define passStartAddr 133
#define passEndAddr 197


void wifiConnect() {
  // Connect to WiFi network
  Serial.println((String)"Trying to connect to: " + ssid + " pass: " + pass);
  WiFi.begin(ssid, pass);
  Serial.println("");

  int atemps = 0;

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED && atemps < 30) {
    delay(500);
    Serial.print(".");
    atemps++;
  }
}

void handleWebServer() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  } else {
    wifiConnect();
  }
}

void eepromInit() {
  EEPROM.begin(512);

  EEPROM_readAnything(tempMinAddr, tempMin);
  EEPROM_readAnything(tempMaxAddr, tempMax);
  EEPROM_readAnything(tempAdjustmentAddr, tempAdjustment);
  EEPROM_readAnything(runModeAddr, runMode);
  ssid = EEPROM_readCharArray(ssidStartAddr, ssidEndAddr);
  pass = EEPROM_readCharArray(passStartAddr, passEndAddr);

}

void returnOK() {
  server.send(200);
}

void fail(String error) {
  errors += error + " | ";
}

void returnFail(String error) {
  server.send(500, "text/json", "{errorcode: " + error + "}");
}

void sendTemp() {
  Serial.println("*******************");
  Serial.println("HTTP_GET a /temp");

  String stringTemp = String(int(temp)) + "." + String(getDecimal(temp));
  String stringTempMin = String(int(tempMin)) + "." + String(getDecimal(tempMin));
  String stringTempMax = String(int(tempMax)) + "." + String(getDecimal(tempMax));
  String stringTempAdjustment = String(int(tempAdjustment)) + "." + String(getDecimal(tempAdjustment));

  String jsonToSend = "{temp: " + stringTemp + ", tempAdjustment: " + stringTempAdjustment + ", tempMin: " + stringTempMin + ", tempMax: " + stringTempMax + "}";

  Serial.println(jsonToSend);
  server.send(200, "text/json", jsonToSend);
}

void handleWifiConf() {
  server.send(200, "text/json", (String) "{SSID:" + ssid + ",PASS:" + pass + "}");
}

void handleEditTempMin() {
  Serial.println("-------------------");
  Serial.println("Edit TempMin");

  if (isNumber(server.arg(TEMP_MIN))) {
    float recievedTempMin = server.arg(TEMP_MIN).toFloat();

    if (recievedTempMin >= tempMax) {
      Serial.println("TempMin > TempMax");
      fail("FAIL - TempMin > TempMax");
    } else {
      tempMin = recievedTempMin;
      EEPROM_writeAnything(tempMinAddr, tempMin);
      Serial.println("OK - New value = " + String(tempMin));
    }
  } else {
    fail("TempMin must be a number");
  }
}

void handleEditTempMax() {
  Serial.println("-------------------");
  Serial.println("Edit TempMax");

  if (isNumber(server.arg(TEMP_MAX))) {
    float recievedTempMax = server.arg(TEMP_MAX).toFloat();

    if (recievedTempMax <= tempMin) {
      Serial.println("TempMax < TempMin");
      fail("FAIL - TempMax < TempMin");
    } else {
      tempMax = recievedTempMax;
      EEPROM_writeAnything(tempMaxAddr, tempMax);
      Serial.println("OK - New value = " + String(tempMax));
    }
  } else {
    fail("TempMax must be a number");
  }
}

void handleTempAdjustment() {
  Serial.println("-------------------");
  Serial.println("Edit TempAdjustment");

  if (isNumber(server.arg(TEMP_ADJUSTMENT))) {
    tempAdjustment = server.arg(TEMP_ADJUSTMENT).toFloat();
    EEPROM_writeAnything(tempAdjustmentAddr, tempAdjustment);
    Serial.println("OK - New value = " + String(tempAdjustment));
  } else {
    fail("TempAdjustment must be a number");
  }
}

void handleEditRunMode() {
  Serial.println("-------------------");
  Serial.println("Edit RunMode");


  if (isNumber(server.arg(RUN_MODE))) {
    float recievedRunMode = server.arg(RUN_MODE).toFloat();

    if (recievedRunMode < 0 || recievedRunMode > 2) {
      Serial.println("Invalid Mode: 0=forcedOFF, 1=normal, 2=forcedON");
      fail("FAIL - Invalid Mode: 0=forcedOFF, 1=normal, 2=forcedON");
    } else {
      runMode = recievedRunMode;
      EEPROM_writeAnything(runModeAddr, runMode);
      Serial.println("OK - New value = " + String(runMode));
    }
  } else {
    fail("RunMode must be a number");
  }
}

void handleConf() {
  Serial.println("*******************");
  Serial.println("HTTP_PUT a /conf");

  if (server.args() == 0) {
    Serial.println("BAD ARGS");
    return returnFail("BAD ARGS");
  }

  if (server.hasArg(TEMP_MIN)) {
    handleEditTempMin();
  }

  if (server.hasArg(TEMP_MAX)) {
    handleEditTempMax();
  }

  if (server.hasArg(TEMP_ADJUSTMENT)) {
    handleTempAdjustment();
  }

  if (server.hasArg(RUN_MODE)) {
    handleEditRunMode();
  }

  if (errors != "") {
    returnFail(errors);
  } else {
    returnOK();
  }
  errors = "";
}

void serverInit() {
  // Si se conecta a la WIFI setea los recursos de configuracion y medicion de temperatura.
  // Si no se conecta levanta en modo access point y levanta recurso para configuracion de datos de WIFI.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/temp", HTTP_GET, sendTemp);
    server.on("/conf", HTTP_PUT, handleConf);

    server.begin();
    Serial.println("HTTP server started");
  } else {
    Serial.println("");
    Serial.println("Failed...");
    WiFi.disconnect();

    WiFi.softAP("ESPWIFI", "11022302");
    Serial.println("");
    Serial.println("Access point ON. SSID: ESPWIFI");

    server.on("/wifiConf", HTTP_PUT, handleWifiConf);

    server.begin();
    Serial.println("HTTP server started");
  }
}

void senseTemp() {
  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0) + tempAdjustment;
}

boolean turnOnRelay() {
  //Primero los modos forzados
  if (runMode == 0) {
    relayON = false;
    return false;
  }
  if (runMode == 2) {
    relayON = true;
    return true;
  }

  //Si no hay modo forzado se calcula en base a la temperatura
  if (temp > tempMax) {
    relayON = true;
    return true;
  }
  if (temp < tempMin) {
    relayON = false;
    return false;
  }

  //Si no pasa nada de lo anterior devuelvo el valor que ya tenia para que siga como estaba
  return relayON;
}

void toogleRelay() {
  if (turnOnRelay()) {
    digitalWrite(relay, 1);
  } else {
    digitalWrite(relay, 0);
  }
}

void setup()
{
  delay(1000);
  eepromInit();

  Serial.begin(9600);
  pinMode(relay, OUTPUT);

  int i = 0;
  while (i < 10) {
    delay(350);
    Serial.print(".");
    i++;
  }

  DS18B20.begin();

  wifiConnect();

  serverInit();
}

void loop()
{
  senseTemp();
  toogleRelay();
  handleWebServer();
}
