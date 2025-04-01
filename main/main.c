#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "shared_commands.h"
#include "espnow_handler.h"
#include "mqtt_client.h"
#include "config_manager.h"
#include "cJSON.h"
#include <inttypes.h>

#define TAG "MQTT_ESPNOW_BRIDGE"

static void handle_espnow_message(const uint8_t *mac_addr, const command_packet_t *cmd) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);
             
    ESP_LOGI(TAG, "Received ESPNOW message from %s: %s", 
            mac_str, command_to_str(cmd->command));
            
    // Convert to JSON and publish to MQTT
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", command_to_str(cmd->command));
    
    switch(cmd->command) {
        case CMD_START:
            // Extract data from the command packet based on the command type
            // This will need to be adjusted based on your actual command_packet_t structure
            break;
            
        case CMD_STATUS:
            // Extract status data
            break;
            
        case CMD_SYNC:
            // Extract sync data
            break;
            
        default:
            break;
    }
    
    char *json = cJSON_PrintUnformatted(root);
    mqtt_publish_status(mac_str, command_to_str(cmd->command), json);
    
    cJSON_Delete(root);
    free(json);
}

static void handle_mqtt_command(const char* mac_str, const char* command, const char* payload) {
    ESP_LOGI(TAG, "Received MQTT command: %s for %s", command, mac_str);
    
    // Parse MAC address
    uint8_t mac[6]; // MAC address is 6 bytes
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    // Parse command
    command_type_t cmd_type = str_to_command(command);
    if (cmd_type == 0) {  // Assuming 0 is an invalid command
        ESP_LOGE(TAG, "Unknown command: %s", command);
        return;
    }
    
    // Create command packet
    // Allocate memory for the command packet with extra space for data
    uint8_t data_size = 0;
    cJSON *root = cJSON_Parse(payload);
    
    // Determine data size based on command type
    if (root) {
        // Calculate data size based on command type and payload
        // This is a simplified example - adjust based on your actual data structures
        switch(cmd_type) {
            case CMD_START:
                data_size = sizeof(start_data_t);
                break;
            case CMD_SYNC:
                data_size = sizeof(sync_data_t);
                break;
            default:
                data_size = 0;
                break;
        }
        
        // Allocate memory for command packet with data
        command_packet_t *cmd = malloc(sizeof(command_packet_t) + data_size);
        if (cmd == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for command packet");
            cJSON_Delete(root);
            return;
        }
        
        cmd->command = cmd_type;
        cmd->data_len = data_size;
        
        // Fill in data based on command type
        switch(cmd_type) {
            case CMD_START:
                if (data_size > 0) {
                    // Fill in start command data
                    // This needs to be adjusted based on your actual data structure
                }
                break;
            case CMD_SYNC:
                if (data_size > 0) {
                    // Fill in sync command data
                    // This needs to be adjusted based on your actual data structure
                }
                break;
            default:
                break;
        }
        
        // Send via ESP-NOW
        espnow_send(mac, cmd);
        
        // Free allocated memory
        free(cmd);
        cJSON_Delete(root);
    } else {
        // If no payload, send a simple command with no data
        command_packet_t cmd = {
            .command = cmd_type,
            .data_len = 0
        };
        espnow_send(mac, &cmd);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    
    // Set log levels
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_ESPNOW_BRIDGE", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
    
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Initialize networking components
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Get MAC address
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    
    // Initialize ESPNOW
    espnow_init(handle_espnow_message);
    
    // Initialize config
    if (!config_manager_init("/spiffs/config.json")) {
        ESP_LOGE(TAG, "Failed to load configuration");
        return;
    }

    // Initialize MQTT with config
    const app_config_t* config = config_manager_get();
    mqtt_client_config_t mqtt_cfg = {
        .uri = config->mqtt_uri,
        .username = config->mqtt_username,
        .password = config->mqtt_password
    };
    ESP_ERROR_CHECK(mqtt_init(handle_mqtt_command, &mqtt_cfg, config->topic_prefix));
    
    // Publish MAC address
    mqtt_publish_mac_address(mac);
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
