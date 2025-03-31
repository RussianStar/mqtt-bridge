#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mqtt.h"
#include "cJSON.h"

#define TAG "MQTT"

static mqtt_command_cb_t command_callback = NULL;
static esp_mqtt_client_handle_t client = NULL;
static char topic_prefix[32] = {0};

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                             int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            // Subscribe to command topics
            char topic[64];
            snprintf(topic, sizeof(topic), "%s/+/commands/#", topic_prefix);
            esp_mqtt_client_subscribe(client, topic, 1);
            break;
            
        case MQTT_EVENT_DATA:
            if (command_callback) {
                // Extract MAC and command from topic
                char *mac_start = strstr(event->topic, topic_prefix) + strlen(topic_prefix) + 1;
                char *mac_end = strchr(mac_start, '/');
                char mac_str[18] = {0};
                strncpy(mac_str, mac_start, mac_end - mac_start);
                
                char *cmd_start = mac_end + strlen("/commands/");
                char *cmd_end = strchr(cmd_start, '/');
                char command[32] = {0};
                strncpy(command, cmd_start, cmd_end - cmd_start);
                
                command_callback(mac_str, command, event->data);
            }
            break;
            
        default:
            break;
    }
}

void mqtt_init(mqtt_command_cb_t command_cb, 
              const esp_mqtt_client_config_t* config,
              const char* prefix) {
    command_callback = command_cb;
    
    client = esp_mqtt_client_init(config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    
    strncpy(topic_prefix, prefix, sizeof(topic_prefix));
}

esp_err_t mqtt_publish_status(const char* mac_str, const char* command, const char* payload) {
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/%s/status/%s/data", topic_prefix, mac_str, command);
    return esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
}
