#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "shared_commands.h"
#include "espnow_handler.h"
#include "custom_mqtt_client.h"
#include "config_manager.h"
#include "cJSON.h"
#include <inttypes.h>
#include <string.h>

// WiFi credentials - replace with your own
#define WIFI_SSID "Tokamabahe!"
#define WIFI_PASSWORD "schneeragout"

// MQTT configuration
#define MQTT_URI "mqtt://192.168.10.34"
#define MQTT_USERNAME "mqtt2"
#define MQTT_PASSWORD "mqttilman"
#define MQTT_TOPIC_PREFIX "pump_controller"

#define TAG "MQTT_ESPNOW_BRIDGE"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
static esp_netif_t *netif_sta = NULL;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

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
    
    // Create event group for WiFi events
    s_wifi_event_group = xEventGroupCreate();
    
    // Create default network interface
    netif_sta = esp_netif_create_default_wifi_sta();
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    
    // Configure WiFi with hardcoded SSID and password
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for WiFi connection or timeout
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000));
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi SSID: %s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi SSID: %s", WIFI_SSID);
        return;
    } else {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return;
    }
    
    // Get MAC address
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    
    // Initialize ESPNOW only after WiFi is connected
    espnow_init(handle_espnow_message);
    
    // Initialize MQTT with hardcoded config
    mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URI,
        .username = MQTT_USERNAME,
        .password = MQTT_PASSWORD
    };
    ESP_ERROR_CHECK(mqtt_init(handle_mqtt_command, &mqtt_cfg, MQTT_TOPIC_PREFIX));
    
    // Publish MAC address
    mqtt_publish_mac_address(mac);
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
