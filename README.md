# MQTT-ESPNOW Bridge

## Overview
This application bridges between MQTT and ESP-NOW protocols for pump control.

## Features
- Relays commands from MQTT to ESP-NOW
- Publishes status updates from ESP-NOW to MQTT
- Supports commands: start, stop, sync, status
- Automatic status reporting on boot
- Enhanced MQTT handling based on ESP-IDF examples
- Dual WiFi mode (AP+STA) for ESP-NOW compatibility

## Setup Instructions

1. Edit the WiFi and MQTT configuration in `main/main.c`:
   ```c
   // WiFi credentials - replace with your own
   #define WIFI_SSID "your_wifi_ssid"
   #define WIFI_PASSWORD "your_wifi_password"

   // MQTT configuration
   #define MQTT_URI "mqtt://192.168.10.34"
   #define MQTT_USERNAME "mqtt2"
   #define MQTT_PASSWORD "mqttilman"
   #define MQTT_TOPIC_PREFIX "pump_controller"
   ```

2. Build and flash the firmware:
   ```bash
   idf.py build
   idf.py -p /dev/ttyUSB0 flash
   ```

3. Monitor the device:
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```

## WiFi and ESP-NOW Configuration

The device operates in dual WiFi mode:
- **Station (STA) mode**: Connects to your WiFi network for MQTT communication
- **Access Point (AP) mode**: Creates an "ESP32_BRIDGE" network for ESP-NOW compatibility

This dual mode configuration allows ESP-NOW to work properly while maintaining WiFi connectivity.

## Topic Structure
- Commands: `{prefix}/{mac}/commands/{command}`
- Status: `{prefix}/{mac}/status/{command}/data`

## Command Protocol
See `components/shared_commands/include/shared_commands.h` for message formats

## Troubleshooting

If you encounter issues with the connection:

1. Verify your WiFi credentials in the main.c file
2. Ensure the MQTT broker is accessible from your network
3. Check the device logs for specific error messages
