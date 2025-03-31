#include "shared_commands.h"
#include "esp_log.h"

#define TAG "COMMANDS"

const char* command_to_str(command_type_t cmd) {
    switch(cmd) {
        case CMD_SYNC: return "SYNC";
        case CMD_START: return "START"; 
        case CMD_STOP: return "STOP";
        case CMD_STATUS: return "STATUS";
        case CMD_RESPONSE: return "RESPONSE";
        default: return "UNKNOWN";
    }
}

command_type_t str_to_command(const char* str) {
    if (!str) return CMD_SYNC;
    
    if (strcmp(str, "SYNC") == 0) return CMD_SYNC;
    if (strcmp(str, "START") == 0) return CMD_START;
    if (strcmp(str, "STOP") == 0) return CMD_STOP;
    if (strcmp(str, "STATUS") == 0) return CMD_STATUS;
    if (strcmp(str, "RESPONSE") == 0) return CMD_RESPONSE;
    
    ESP_LOGW(TAG, "Unknown command string: %s", str);
    return CMD_SYNC;
}

// Helper to convert valve states to bitfield
uint8_t valves_to_bitfield(valve_state_t states[3]) {
    uint8_t bitfield = 0;
    for (int i = 0; i < 3; i++) {
        if (states[i] == VALVE_ON) {
            bitfield |= (1 << i);
        }
    }
    return bitfield;
}

// Helper to convert bitfield to valve states
void bitfield_to_valves(uint8_t bitfield, valve_state_t states[3]) {
    for (int i = 0; i < 3; i++) {
        states[i] = (bitfield & (1 << i)) ? VALVE_ON : VALVE_OFF;
    }
}
