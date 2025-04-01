#include "espnow_handler.h"
#include "shared_commands.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <string.h>

#define TAG "ESPNOW"

static espnow_receive_cb_t receive_callback = NULL;

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (receive_callback && info && info->src_addr && data && len >= sizeof(command_packet_t)) {
        command_packet_t* cmd = (command_packet_t*)data;
        
        // Validate data length
        if (len >= sizeof(command_packet_t) + cmd->data_len) {
            receive_callback(info->src_addr, cmd);
        } else {
            ESP_LOGE(TAG, "Received invalid data length: %d, expected at least: %d", 
                    len, sizeof(command_packet_t) + cmd->data_len);
        }
    } else {
        ESP_LOGE(TAG, "Received invalid ESPNOW message");
    }
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGD(TAG, "Send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void espnow_init(espnow_receive_cb_t receive_cb) {
    // Store callback even if it's NULL
    receive_callback = receive_cb;
    
    // De-init ESP-NOW first in case it was already initialized
    esp_now_deinit();
    
    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    
    // Add broadcast peer
    esp_now_peer_info_t peer = {0};
    memset(peer.peer_addr, 0xFF, ESP_NOW_ETH_ALEN); // Broadcast address
    peer.channel = 0;
    peer.ifidx = ESP_IF_WIFI_AP;
    peer.encrypt = false;
    
    // Add peer or update if it exists
    esp_err_t err = esp_now_add_peer(&peer);
    if (err == ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGI(TAG, "Broadcast peer already exists, updating");
        ESP_ERROR_CHECK(esp_now_mod_peer(&peer));
    } else {
        ESP_ERROR_CHECK(err);
    }
    
    ESP_LOGI(TAG, "ESP-NOW initialized successfully");
}

esp_err_t espnow_send(const uint8_t *mac_addr, const command_packet_t *cmd) {
    if (mac_addr == NULL || cmd == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for espnow_send");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Calculate total size
    size_t total_size = sizeof(command_packet_t) + cmd->data_len;
    
    // Log sending information
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);
    ESP_LOGI(TAG, "Sending command %d to %s, data size: %u", 
             cmd->command, mac_str, cmd->data_len);
    
    // Send the command
    esp_err_t err = esp_now_send(mac_addr, (uint8_t *)cmd, total_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send ESP-NOW message: %s", esp_err_to_name(err));
    }
    
    return err;
}
