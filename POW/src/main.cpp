// case study and basic routines for reading SONOFF_POW by Reinhard Nickels - please visit my blog http://glaskugelsehen.de
// for SonOFF POW set Core Dev Module, Flash 1M, 64k SPIFFS
// thanks to Xose Pérez for his great work on the HLW8012 lib and his inspiration for this code
// basic routines from ArduinoOTA lib are included

#include <HLW8012.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// GPIOs
#define RELAY_PIN     12
#define LED_PIN       15
#define BUTTON_PIN    0
#define SEL_PIN       5
#define CF1_PIN       13
#define CF_PIN        14

#define UPDATE_TIME                     10000   // Check values every 10 seconds
#define CURRENT_MODE                    HIGH
// These are the nominal values for the resistors in the circuit
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 464000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 983 ) // Real 1.009k

char messag[125] = "";
char str_temp[12] = "";
const char* password = "Roma2045";
const char* ssid = "The Papaya Group";
const char* host = "192.168.5.40";   // TCP Client for websoccket connection
const char* mqtt_server = "192.168.5.186";
const int port = 1883;

const char* mqtt_id = "TPG-sonoff";
const char* sub_channel = "/sonoff";
const char* pub_channel = "/node";
const int httpPort = 61952;
unsigned long lastmillis;
int counter;
unsigned long buttontimer;

WiFiUDP udp;
HLW8012 hlw8012;
WiFiClient client;
PubSubClient mqtt(client);
DynamicJsonBuffer jsonAnswer;
JsonObject& answer = jsonAnswer.createObject();
JsonObject& device = answer.createNestedObject("device");
JsonObject& measures = answer.createNestedObject("measures");


boolean wait_for_brelease = false;

// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void ICACHE_RAM_ATTR hlw8012_cf1_interrupt() {
    hlw8012.cf1_interrupt();
}
void ICACHE_RAM_ATTR hlw8012_cf_interrupt() {
    hlw8012.cf_interrupt();
}

// Library expects an interrupt on both edges
void setInterrupts() {
    attachInterrupt(CF1_PIN, hlw8012_cf1_interrupt, CHANGE);
    attachInterrupt(CF_PIN, hlw8012_cf_interrupt, CHANGE);
}

void calibrate() {

    // Let some time to register values
    unsigned long timeout = millis();
    while ((millis() - timeout) < 10000) {
        delay(1);
    }

    // Calibrate using a 60W bulb (pure resistive) on a 230V line
    hlw8012.expectedActivePower(232*0.169);
    hlw8012.expectedVoltage(231.0);
    hlw8012.expectedCurrent(0.169);
    /* 4 is mininum width, 2 is precision; float value is copied onto str_temp*/
    dtostrf(hlw8012.getCurrentMultiplier(), 4, 2, str_temp);
    // Show corrected factors
    sprintf(messag,"[HLW] New current multiplier    : %s",str_temp);
    udp.beginPacket(host,httpPort);
    udp.print(messag);
    udp.endPacket();

    dtostrf(hlw8012.getVoltageMultiplier(), 4, 2, str_temp);
    sprintf(messag,"[HLW] New voltage multiplier    : %s",str_temp);
    udp.beginPacket(host,httpPort);
    udp.print(messag);
    udp.endPacket();

    dtostrf(hlw8012.getPowerMultiplier(), 4, 2, str_temp);
    sprintf(messag,"[HLW] New power multiplier    : %s",str_temp);
    udp.beginPacket(host,httpPort);
    udp.print(messag);
    udp.endPacket();
}

void callback(char* topic, byte* payload, unsigned int length) {
    char p[length+1];
    memcpy(p,payload,length);
    p[length]=NULL;
    String message(p);
    Serial.println(message);
    digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  mqtt.setServer(mqtt_server,port);
  mqtt.setCallback(callback);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));    // toggle
  }
  while (!mqtt.connect(mqtt_id,"papaya","sodio8potasio12")) {
      Serial.print(".");
      delay(500);
  }
  mqtt.publish((char*)sub_channel,(char*)"connected");
  mqtt.subscribe(pub_channel);

  digitalWrite(LED_PIN, HIGH);
  hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true);

  // These values are used to calculate current, voltage and power factors as per datasheet formula
  // These are the nominal values for the Sonoff POW resistors:
  // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
  // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
  // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
  hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);
  hlw8012.setCurrentMultiplier(14143.95);   // change to determined values
  hlw8012.setVoltageMultiplier(433146.11); //442361.98  433146.11  423930.24
  hlw8012.setPowerMultiplier(10902725.89);
  // Show default (as per datasheet) multipliers

  dtostrf(hlw8012.getCurrentMultiplier(), 4, 2, str_temp);
  sprintf(messag,"[HLW] Default current multiplier    : %s",str_temp);
  udp.beginPacket(host,httpPort);
  udp.print(messag);
  udp.endPacket();

  dtostrf(hlw8012.getVoltageMultiplier(), 4, 2, str_temp);
  sprintf(messag,"[HLW] Default voltage multiplier    : %s",str_temp);
  udp.beginPacket(host,httpPort);
  udp.print(messag);
  udp.endPacket();

  dtostrf(hlw8012.getPowerMultiplier(), 4, 2, str_temp);
  sprintf(messag,"[HLW] Default power multiplier    : %s",str_temp);
  udp.beginPacket(host,httpPort);
  udp.print(messag);
  udp.endPacket();

  setInterrupts();
  digitalWrite(RELAY_PIN, HIGH);
  delay(5000);
  calibrate();
}

void loop() {
    mqtt.loop();
    delay(10); //fixes some issues with WiFi stability
    if(!mqtt.connected()) {
      while (!mqtt.connect(mqtt_id,"papaya","sodio8potasio12")) {
          Serial.print(".");
          delay(500);
      }
    }
  // check if button was pressed, debounce and toggle relais
    if (!digitalRead(BUTTON_PIN) && buttontimer == 0 && !wait_for_brelease) {
      buttontimer = millis();
    }
    else if (!digitalRead(BUTTON_PIN) && buttontimer != 0) {
      if (millis() - buttontimer > 20 && !wait_for_brelease) {
        digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));

        buttontimer = 0;
        wait_for_brelease = true;
      }
    }
    if (wait_for_brelease) {
      if (digitalRead(BUTTON_PIN)) wait_for_brelease = false;
    }
    if (millis() - lastmillis > UPDATE_TIME) {
      lastmillis = millis();
      /* 4 is mininum width, 2 is precision; float value is copied onto str_temp*/
      dtostrf(hlw8012.getActivePower(), 4, 2, str_temp);
      sprintf(messag,"[HLW] Active Power (W)    : %s",str_temp);
      udp.beginPacket(host,httpPort);
      udp.print(messag);
      udp.endPacket();

      dtostrf(hlw8012.getVoltage(), 4, 2, str_temp);
      sprintf(messag,"[HLW] Voltage (V)         : %s",str_temp);
      udp.beginPacket(host,httpPort);
      udp.print(messag);
      udp.endPacket();

      dtostrf(hlw8012.getCurrent(), 4, 2, str_temp);
      sprintf(messag,"[HLW] Current (A)         : %s",str_temp);
      udp.beginPacket(host,httpPort);
      udp.print(messag);
      udp.endPacket();

      dtostrf(hlw8012.getApparentPower(), 4, 2, str_temp);
      sprintf(messag,"[HLW] Apparent Power (VA) : %s",str_temp);
      udp.beginPacket(host,httpPort);
      udp.print(messag);
      udp.endPacket();

      dtostrf(hlw8012.getPowerFactor(), 4, 2, str_temp);
      sprintf(messag,"[HLW] Power Factor (%)    : %s",str_temp);
      udp.beginPacket(host,httpPort);
      udp.print(messag);
      udp.endPacket();

      dtostrf(hlw8012.getEnergy(), 4, 5, str_temp);
      sprintf(messag,"[HLW] Energy (W/s)    : %s",str_temp);
      udp.beginPacket(host,httpPort);
      udp.print(messag);
      udp.endPacket();

      mqtt.publish("/hello", "world");
      device["id"] = "sonoff-pow";
      measures["voltage"] = hlw8012.getVoltage();
      measures["current"] = hlw8012.getCurrent();
      measures["activepower"] = hlw8012.getActivePower();
      measures["aparentpower"] = hlw8012.getApparentPower();
      measures["powerfactor"] = hlw8012.getPowerFactor();
      measures["energy"] = hlw8012.getEnergy();
      char answerJson[256];
      answer.printTo(answerJson, sizeof(answerJson));
      mqtt.publish("/device/room/printer", answerJson);
      hlw8012.resetEnergy();
  }
}
