#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
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
#define ESP_SSID "espSSID"
#define ESP_PASS "espPASS"

// WIFI DECLARATION
char* ssid;
char* password;
ESP8266WebServer server(80);
String errors = "";

struct WifiData {
  char ssid[32];
  char pass[64];
};


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
#define wifiDataAddr 100

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
  server.handleClient();
  senseTemp();
  toogleRelay();
  checkConnection();
}

void eepromInit() {
  EEPROM.begin(512);

  EEPROM_readAnything(tempMinAddr, tempMin);
  EEPROM_readAnything(tempMaxAddr, tempMax);
  EEPROM_readAnything(tempAdjustmentAddr, tempAdjustment);
  EEPROM_readAnything(runModeAddr, runMode);

//  WifiData wd1;
//  String ssid2 = "Munis Red";
//  String pass2 = "32144584";
//  ssid2.toCharArray(wd1.ssid, 32);
//  pass2.toCharArray(wd1.pass, 64);
//  
//  Serial.println((String)"Escribiendo: " + wd1.ssid + " |||| " + wd1.pass);
//
//  EEPROM_writeAnything(wifiDataAddr, wd1);

  WifiData wifiData;
  EEPROM_readAnything(wifiDataAddr, wifiData);
  ssid = wifiData.ssid;
  password = wifiData.pass;
  Serial.println((String)"SSID: " + ssid + " | PASS: " + password);
}

void wifiConnect() {
  // Connect to WiFi network
  Serial.println((String)"Trying to connect to: " + ssid + " pass: " + password);
  WiFi.begin(ssid, password);
  Serial.println("");

  int atemps = 0;

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED && atemps < 30) {
    delay(500);
    Serial.print(".");
    atemps++;
  }
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

void handleWifiConf() {
  Serial.println("-------------------");
  Serial.println("Edit Wifi Config");

  String recievedSSID;
  String recievedPASS;

  if (server.args() == 0) {
    Serial.println("BAD ARGS");
    return returnFail("BAD ARGS");
  }

  if (server.hasArg(ESP_SSID)) {
    recievedSSID = server.arg(ESP_SSID);
    if (recievedSSID == "") {
      Serial.println("SSID can't be null");
      return returnFail("SSID can't be null");
    }
  }

  if (server.hasArg(ESP_PASS)) {
    recievedPASS = server.arg(ESP_PASS);
    if (recievedPASS == "") {
      Serial.println("PASS can't be null");
      return returnFail("PASS can't be null");
    }
  }

  WifiData wifiData;

  recievedSSID.toCharArray(wifiData.ssid, 32);
  recievedPASS.toCharArray(wifiData.pass, 64);

  Serial.println((String)wifiData.ssid + " " + wifiData.pass);

  EEPROM_writeAnything(wifiDataAddr, wifiData);

  Serial.println("OK - New SSID = " + recievedSSID + " | New PASS = " + recievedPASS);

  returnOK();
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

void returnOK() {
  server.send(200);
}

void fail(String error) {
  errors += error + " | ";
}

void returnFail(String error) {
  server.send(500, "text/json", "{errorcode: " + error + "}");
}

void senseTemp() {

  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0) + tempAdjustment;
}

void toogleRelay() {

  if (turnOnRelay()) {
    digitalWrite(relay, 1);
  } else {
    digitalWrite(relay, 0);
  }
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

  //Si no pasa nada de lo anterior devuelvo true
  //(significa que la temp tiene que seguir bajando)
  return relayON;
}
