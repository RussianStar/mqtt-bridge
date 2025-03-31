# MQTT-ESPNOW Bridge

## Setup Instructions

1. Flash the firmware using idf.py:
   ```
   idf.py build flash
   ```

2. Create a config file at `/spiffs/config.json` with this format:
   ```json
   {
       "mqtt": {
           "uri": "mqtt://your.broker.com",
           "username": "your_username",
           "password": "your_password"
       },
       "topics": {
           "prefix": "pump_controller"
       }
   }
   ```

3. Features:
   - Bridges MQTT and ESP-NOW protocols
   - Supports all pump controller commands
   - JSON message formatting
   - Configurable via config file

4. Topic Structure:
   - Commands: `{prefix}/{mac}/commands/{command}`
   - Status: `{prefix}/{mac}/status/{command}/data`

## Overview
This application bridges between MQTT and ESP-NOW protocols for pump control.

## Setup
1. Edit `config/config.json` with your MQTT broker details
2. Build and flash with:
```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Features
- Relays commands from MQTT to ESP-NOW
- Publishes status updates from ESP-NOW to MQTT
- Supports commands: start, stop, sync, status
- Automatic status reporting on boot

## Configuration
- MQTT broker settings in config.json
- Topic format: `<prefix>/<mac>/<command>/data`

## Command Protocol
See `components/shared_commands/include/shared_commands.h` for message formats
