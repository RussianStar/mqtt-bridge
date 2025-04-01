# MQTT-ESPNOW Bridge

## Overview
This application bridges between MQTT and ESP-NOW protocols for pump control.

## Features
- Relays commands from MQTT to ESP-NOW
- Publishes status updates from ESP-NOW to MQTT
- Supports commands: start, stop, sync, status
- Automatic status reporting on boot
- WiFi and MQTT configuration via config file
- SPIFFS storage for configuration

## Setup Instructions

1. Edit `config/config.json` with your MQTT broker and WiFi details:
   ```json
   {
       "mqtt": {
           "uri": "mqtt://your.broker.com",
           "username": "your_username",
           "password": "your_password"
       },
       "topics": {
           "prefix": "pump_controller"
       },
       "wifi": {
           "ssid": "your_wifi_ssid",
           "password": "your_wifi_password"
       }
   }
   ```

2. Create a SPIFFS image with your configuration:
   - On Linux/macOS:
     ```bash
     chmod +x create_spiffs_image.sh
     ./create_spiffs_image.sh
     ```
   - On Windows:
     ```
     create_spiffs_image.bat
     ```

3. Build and flash the firmware:
   ```bash
   idf.py build
   idf.py -p /dev/ttyUSB0 flash
   ```

4. Flash the SPIFFS partition with your configuration:
   ```bash
   esptool.py --port /dev/ttyUSB0 write_flash 0x310000 spiffs.bin
   ```

5. Monitor the device:
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```

## Topic Structure
- Commands: `{prefix}/{mac}/commands/{command}`
- Status: `{prefix}/{mac}/status/{command}/data`

## Command Protocol
See `components/shared_commands/include/shared_commands.h` for message formats

## Troubleshooting

If you encounter issues with the configuration:

1. Check that the SPIFFS partition is correctly flashed
2. Verify your WiFi credentials in the config.json file
3. Ensure the MQTT broker is accessible from your network
4. Check the device logs for specific error messages
