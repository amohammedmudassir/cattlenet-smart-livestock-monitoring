#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"

// -------------------- WiFi credentials --------------------
const char* ssid = "Nowifi";
const char* password = "23456789";

// -------------------- MQTT broker settings --------------------
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USERNAME";     // optional
const char* mqtt_password = "YOUR_MQTT_PASSWORD"; // optional
const char* topic = "farm/feed_monitor";

// -------------------- Pin definitions --------------------
#define LOADCELL_DOUT_PIN 12  // D6
#define LOADCELL_SCK_PIN 13   // D7
#define FLOAT_SWITCH_PIN 5    // D1

// -------------------- Objects --------------------
WiFiClient espClient;
PubSubClient client(espClient);
HX711 scale;

// -------------------- Load cell calibration --------------------
float calibration_factor = -1025;  // your determined calibration factor

// --------------------------------------------------------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int retry_count = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry_count++;
    if (retry_count > 40) {
      Serial.println("WiFi connection failed, restarting...");
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// --------------------------------------------------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("D1FeedMonitor", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

// --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Initialize HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(2000);

  scale.set_scale(calibration_factor);  // apply your calibration factor
  scale.tare();                         // reset scale to zero

  Serial.println("Taring... Please ensure no weight on the scale");
  delay(2000);
  scale.tare();                         // double-tare for accuracy

  // Initialize float switch
  pinMode(FLOAT_SWITCH_PIN, INPUT_PULLUP);

  Serial.println("System Initialized and Ready!");
}

// --------------------------------------------------------------
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  bool waterPresent = (digitalRead(FLOAT_SWITCH_PIN) == LOW);  // LOW = water present
  float weight = 0;

  if (scale.is_ready()) {
    // Take multiple readings for stable output
    long sum = 0;
    const int samples = 5;
    for (int i = 0; i < samples; i++) {
      sum += scale.get_units(10);  // average of 10 readings each
      delay(100);
    }
    weight = sum / (float)samples;

    // Prevent small negative drift values
    if (weight < 0.05 && weight > -0.05) weight = 0;
  } else {
    Serial.println("HX711 not ready");
  }

  // Prepare JSON payload
  String payload = "{";
  payload += "\"weight\":";
  payload += String(weight, 2);
  payload += ",";
  payload += "\"waterPresent\":";
  payload += (waterPresent ? "true" : "false");
  payload += "}";

  // Publish to MQTT
  if (client.publish(topic, payload.c_str())) {
    Serial.print("Published: ");
    Serial.println(payload);
  } else {
    Serial.println("Publish failed");
  }

  delay(2000);  // publish every 2 seconds
}
