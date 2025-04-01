#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "esp_err.h"
#include "mqtt_client.h"
#include "shared_commands.h"

// Custom MQTT client configuration type
typedef struct {
    const char* uri;
    const char* username;
    const char* password;
} mqtt_client_config_t;

typedef void (*mqtt_command_cb_t)(const char* mac_str, const char* command, const char* payload);

void mqtt_init(mqtt_command_cb_t command_cb, 
              const mqtt_client_config_t* config,
              const char* topic_prefix);
esp_err_t mqtt_publish_status(const char* mac_str, const char* command, const char* payload);
void mqtt_publish_mac_address(const uint8_t mac[6]);

#endif // MQTT_CLIENT_H
