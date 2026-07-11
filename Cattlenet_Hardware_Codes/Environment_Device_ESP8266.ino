#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Pin definitions
#define DHT_PIN  D2
#define LDR_PIN  A0
#define TRIG_PIN D5
#define ECHO_PIN D6
#define BUZZER_PIN D7  // Buzzer +ve connected here, negative to GND

// Constants
#define DHTTYPE DHT11
#define DAY_THRESHOLD 500   // Adjust as per your calibration

// WiFi credentials
const char* ssid = "CMRIT CAMPUS WIFI";
const char* password = "password";

// MQTT broker details
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "farm/environment";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHTTYPE);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

long getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // Timeout 30 ms
  if (duration == 0) return -1;  // No pulse received => out of range
  long distanceCm = duration * 0.034 / 2;
  return distanceCm;
}

void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Buzzer off initially (active HIGH)

  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Setup complete. Starting sensor readings...");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int ldrValue = analogRead(LDR_PIN);
  bool isDay = (ldrValue < DAY_THRESHOLD);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  long distance = getDistanceCm();

  String cattlePresence;
  if (distance > 0 && distance < 50) {
    digitalWrite(BUZZER_PIN, HIGH);  // Buzzer ON (active HIGH)
    cattlePresence = "Cattle detected";
  } else {
    digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF
    if (distance == -1) {
      cattlePresence = "Out of range";
    } else {
      cattlePresence = "No cattle detected";
    }
  }

  // Format JSON payload excluding food recommendation
  String payload = "{";
  payload += "\"ldrValue\":" + String(ldrValue) + ",";
  payload += "\"isDay\":\"" + String(isDay ? "Day" : "Night") + "\",";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"cattlePresence\":\"" + cattlePresence + "\"";
  payload += "}";

  Serial.print("Publishing: ");
  Serial.println(payload);

  client.publish(mqtt_topic, payload.c_str());

  delay(2000);
}
