#ifndef SHARED_COMMANDS_H
#define SHARED_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_system.h"

// Command types
typedef enum {
    CMD_SYNC = 0x01,
    CMD_START = 0x02,
    CMD_STOP = 0x03,
    CMD_STATUS = 0x04,
    CMD_RESPONSE = 0x80  // MSB set for responses
} command_type_t;

// Valve states
typedef enum {
    VALVE_OFF = 0,
    VALVE_ON = 1,
    VALVE_UNKNOWN = 2
} valve_state_t;

// Pump operation status
typedef enum {
    PUMP_INACTIVE = 0,
    PUMP_ACTIVE = 1,
    PUMP_UNKNOWN = 2
} pump_status_t;

// Configuration structure
typedef struct {
    uint8_t master_mac[6];  // MAC address of master device
    int pump_gpio;          // GPIO for pump control
    struct {
        int gpio;
        bool default_state;  // Default state (true = open, false = closed)
    } valves[3];            // GPIOs for 3 valves
    char timezone[32];      // Timezone string
} pump_config_t;

// Status structure
typedef struct {
    time_t current_time;     // Current time on the device
    float battery_soc;       // State of charge (0.0-100.0)
    pump_status_t pump_state; // Pump operation status
    valve_state_t valve_states[3]; // Status of each valve
} pump_status_data_t;

// Command packet structure
typedef struct __attribute__((packed)) {
    uint8_t command;         // Command type
    uint8_t data_len;        // Length of data
    uint8_t data[0];         // Variable length data
} command_packet_t;

// Sync command data structure
typedef struct __attribute__((packed)) {
    time_t device_time;      // Current time on the device
    float battery_soc;       // State of charge
} sync_data_t;

// Sync response data structure
typedef struct __attribute__((packed)) {
    time_t master_time;      // Current time on the master
} sync_response_t;

// Start command data structure
typedef struct __attribute__((packed)) {
    uint32_t duration_sec;   // Duration in seconds
    uint8_t valve_control;   // Bit field for valve control
    uint8_t valve_states;    // Bit field for valve states
} start_data_t;

// Status response data structure
typedef struct __attribute__((packed)) {
    time_t device_time;      // Current time on the device
    float battery_soc;       // State of charge
    uint8_t pump_state;      // Pump operation status
    uint8_t valve_states;    // Bit field for valve states
} status_response_t;

// Helper functions
const char* command_to_str(command_type_t cmd);
command_type_t str_to_command(const char* str);

#endif /* SHARED_COMMANDS_H */
