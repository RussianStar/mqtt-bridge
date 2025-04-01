#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_event.h"
#include <string.h>

#define TAG "MQTT"

static esp_mqtt_client_handle_t client;
static char topic_prefix[64];
static mqtt_command_cb_t command_callback;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                             int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch(event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            break;
        case MQTT_EVENT_DATA:
            if (command_callback) {
                char topic[256];
                strncpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';
                
                char data[256];
                strncpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';
                
                // Extract MAC from topic
                char *mac_start = strstr(topic, topic_prefix);
                if (mac_start) {
                    mac_start += strlen(topic_prefix) + 1;
                    char *mac_end = strchr(mac_start, '/');
                    if (mac_end) {
                        char mac_str[18];
                        strncpy(mac_str, mac_start, mac_end - mac_start);
                        mac_str[mac_end - mac_start] = '\0';
                        
                        // Extract command
                        char *cmd_start = strstr(topic, "commands/");
                        if (cmd_start) {
                            cmd_start += 9; // length of "commands/"
                            command_callback(mac_str, cmd_start, data);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}

esp_err_t mqtt_init(mqtt_command_cb_t command_cb, 
              const mqtt_client_config_t* config,
              const char* prefix) {
    command_callback = command_cb;
    strncpy(topic_prefix, prefix, sizeof(topic_prefix)-1);
    
    esp_mqtt_client_config_t esp_mqtt_cfg = {};
    esp_mqtt_cfg.broker.uri = config->uri;
    esp_mqtt_cfg.credentials.username = config->username;
    esp_mqtt_cfg.credentials.password = config->password;
    
    client = esp_mqtt_client_init(&esp_mqtt_cfg);
    if (client == NULL) {
        return ESP_FAIL;
    }
    
    esp_err_t err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }
    
    return esp_mqtt_client_start(client);
}

esp_err_t mqtt_publish_status(const char* mac_str, const char* command, const char* payload) {
    char topic[256];
    snprintf(topic, sizeof(topic), "%s/%s/status/%s/data", topic_prefix, mac_str, command);
    return esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
}

void mqtt_publish_mac_address(const uint8_t mac[6]) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/bridge/mac", topic_prefix);

    esp_mqtt_client_publish(client, topic, mac_str, 0, 1, 0);
    ESP_LOGI(TAG, "Published MAC address: %s", mac_str);
}
