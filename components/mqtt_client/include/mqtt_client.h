#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "esp_err.h"
#include "esp_mqtt.h"
#include "shared_commands.h"

typedef void (*mqtt_command_cb_t)(const char* mac_str, const char* command, const char* payload);

void mqtt_init(mqtt_command_cb_t command_cb, 
              const esp_mqtt_client_config_t* config,
              const char* topic_prefix);
esp_err_t mqtt_publish_status(const char* mac_str, const char* command, const char* payload);

#endif // MQTT_CLIENT_H
