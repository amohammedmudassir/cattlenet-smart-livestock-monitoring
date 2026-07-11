import serial
import paho.mqtt.client as mqtt
import time
import json

# Configuration - update SERIAL_PORT accordingly
SERIAL_PORT = "COM7"       # e.g. 'COM7' on Windows, '/dev/ttyACM0' on Linux/macOS
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
MQTT_TOPIC = "farm/gate"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT Broker!")
    else:
        print("❌ Failed to connect, return code", rc)

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2) # wait for Arduino reset
        print(f"Connected to serial port {SERIAL_PORT}")
    except Exception as e:
        print("Error opening serial port:", e)
        return

    client = mqtt.Client()
    client.on_connect = on_connect
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
    except Exception as e:
        print("Error connecting to MQTT broker:", e)
        return

    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print("Serial Data:", line)
                # Optional: validate JSON before publishing
                try:
                    json.loads(line)
                    client.publish(MQTT_TOPIC, line)
                    print(f"Published to {MQTT_TOPIC}")
                except json.JSONDecodeError:
                    print("Invalid JSON, skipping:", line)
        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print("Error:", e)
            time.sleep(2)

    ser.close()
    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    main()
