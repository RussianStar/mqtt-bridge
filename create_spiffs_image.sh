#!/bin/bash

# Create a temporary directory for SPIFFS files
mkdir -p spiffs_files

# Copy the config file to the SPIFFS directory
cp config/config.json spiffs_files/

# Create the SPIFFS image
python $IDF_PATH/components/spiffs/spiffsgen.py 0xF0000 spiffs_files/ spiffs.bin

# Clean up
rm -rf spiffs_files

echo "SPIFFS image created: spiffs.bin"
echo "Flash it using: esptool.py --port [PORT] write_flash 0x310000 spiffs.bin"
