# CattleNet — Hardware

Embedded firmware and gateway scripts for CattleNet, a solar-powered IoT livestock monitoring system. This folder contains microcontroller code for environment sensing, feed monitoring, and automated gate control, all communicating over MQTT.

## Devices

### 1. Environment Monitoring Device (`Environment_Device_ESP8266.ino`)
Runs on ESP8266. Reads livestock shed environmental data and publishes readings over MQTT.
- Sensors: [e.g. DHT22 for temperature/humidity, add yours]
- Publishes to topic: `[your topic name]`
- Update interval: `[e.g. every 30s]`

### 2. Feed Monitoring Device (`Feed_Monitoring_Device_ESP8266.ino`)
Runs on ESP8266. Monitors feed levels/consumption and publishes status over MQTT.
- Sensors: `[e.g. ultrasonic/load cell]`
- Publishes to topic: `[your topic name]`

### 3. Gate Control Device (`Gate_Device_Arduino.ino`)
Runs on Arduino. Controls automated gate access, triggered via serial commands from the gateway script.
- Actuator: `[e.g. servo/relay]`
- Communicates via: Serial (USB/UART) with `serial_to_mqtt_Gate_Device.py`

### 4. Serial-to-MQTT Bridge (`serial_to_mqtt_Gate_Device.py`)
Python script that bridges the Arduino gate device (serial) to the MQTT broker, since the gate Arduino lacks native WiFi.
- Reads serial output from `Gate_Device_Arduino.ino`
- Publishes/subscribes on MQTT topics: `[list topics]`
- Run with: `python serial_to_mqtt_Gate_Device.py`

## Architecture
[Environment Sensor] --MQTT--> Broker
[Feed Sensor]        --MQTT--> Broker  --> Backend (see /software)
[Gate Arduino] --Serial--> [Bridge Script] --MQTT--> Broker

## Requirements
- Arduino IDE (for `.ino` files) with ESP8266 board package installed
- Python 3.x with `paho-mqtt` and `pyserial` for the bridge script:
pip install paho-mqtt pyserial

## Setup
1. Flash `Environment_Device_ESP8266.ino` and `Feed_Monitoring_Device_ESP8266.ino` to respective ESP8266 boards via Arduino IDE.
2. Update WiFi credentials and MQTT broker address in each `.ino` file before flashing.
3. Flash `Gate_Device_Arduino.ino` to the Arduino controlling the gate.
4. Connect the gate Arduino to a host machine (Raspberry Pi/laptop) via USB and run `serial_to_mqtt_Gate_Device.py` to bridge it to MQTT.

## Related
Backend and dashboard code: [`/Cattlenet_Software`](../Cattlenet_Software)
Live demo: https://cattle-net.vercel.app
