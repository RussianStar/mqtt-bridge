#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "shared_commands.h"
#include "espnow_handler.h"

#define TAG "MQTT_ESPNOW_BRIDGE"

static void handle_espnow_message(const esp_now_message_t *message) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             message->mac[0], message->mac[1], message->mac[2],
             message->mac[3], message->mac[4], message->mac[5]);
             
    ESP_LOGI(TAG, "Received ESPNOW message from %s: %s", 
            mac_str, command_to_str(message->command));
            
    // Convert to JSON and publish to MQTT
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", command_to_str(message->command));
    
    switch(message->command) {
        case CMD_START_PUMP:
            cJSON_AddNumberToObject(root, "duration", message->data.start.duration);
            cJSON_AddBoolToObject(root, "valve1", message->data.start.valve1);
            cJSON_AddBoolToObject(root, "valve2", message->data.start.valve2);
            cJSON_AddBoolToObject(root, "valve3", message->data.start.valve3);
            break;
            
        case CMD_STATUS:
            cJSON_AddStringToObject(root, "time", message->data.status.time);
            cJSON_AddStringToObject(root, "current_mode", 
                                  command_to_str(message->data.status.current_mode));
            cJSON_AddNumberToObject(root, "battery_soc", message->data.status.battery_soc);
            break;
            
        case CMD_SYNC_TIME:
            cJSON_AddStringToObject(root, "time", message->data.sync.time);
            break;
            
        default:
            break;
    }
    
    char *json = cJSON_PrintUnformatted(root);
    mqtt_publish_status(mac_str, command_to_str(message->command), json);
    
    cJSON_Delete(root);
    free(json);
}

#include "mqtt_client.h"
#include "cJSON.h"

static void handle_mqtt_command(const char* mac_str, const char* command, const char* payload) {
    ESP_LOGI(TAG, "Received MQTT command: %s for %s", command, mac_str);
    
    // Parse MAC address
    uint8_t mac[MAC_ADDR_LEN];
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    // Parse command
    pump_command_t cmd = str_to_command(command);
    if (cmd == CMD_UNKNOWN) {
        ESP_LOGE(TAG, "Unknown command: %s", command);
        return;
    }
    
    // Create ESP-NOW message
    esp_now_message_t msg;
    memcpy(msg.mac, mac, MAC_ADDR_LEN);
    msg.command = cmd;
    
    // Parse payload if needed
    cJSON *root = cJSON_Parse(payload);
    if (root) {
        switch(cmd) {
            case CMD_START_PUMP:
                msg.data.start.duration = cJSON_GetObjectItem(root, "duration")->valueint;
                msg.data.start.valve1 = cJSON_GetObjectItem(root, "valve1")->valueint;
                msg.data.start.valve2 = cJSON_GetObjectItem(root, "valve2")->valueint;
                msg.data.start.valve3 = cJSON_GetObjectItem(root, "valve3")->valueint;
                break;
                
            case CMD_SYNC_TIME:
                strncpy(msg.data.sync.time, 
                       cJSON_GetObjectItem(root, "time")->valuestring,
                       TIME_STR_LEN);
                break;
                
            default:
                break;
        }
        cJSON_Delete(root);
    }
    
    // Send via ESP-NOW
    espnow_send(mac, &msg);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting MQTT-ESPNOW Bridge");
    
    // Initialize WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Initialize ESPNOW
    espnow_init(handle_espnow_message);
    
    // Initialize config
    if (!config_init("/spiffs/config.json")) {
        ESP_LOGE(TAG, "Failed to load configuration");
        return;
    }

    // Initialize MQTT with config
    const app_config_t* cfg = config_get();
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = cfg->mqtt_uri,
        .username = cfg->mqtt_username,
        .password = cfg->mqtt_password
    };
    mqtt_init(handle_mqtt_command, &mqtt_cfg, cfg->topic_prefix);
    
    // TODO: Send initial status
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
