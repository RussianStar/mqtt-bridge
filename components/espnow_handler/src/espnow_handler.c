#include "espnow_handler.h"
#include "shared_commands.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <string.h>

#define TAG "ESPNOW"

static espnow_receive_cb_t receive_callback = NULL;

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (receive_callback) {
        command_packet_t* cmd = (command_packet_t*)data;
        receive_callback(info->src_addr, cmd);
    }
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGD(TAG, "Send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void espnow_init(espnow_receive_cb_t receive_cb) {
    receive_callback = receive_cb;
    
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
}

esp_err_t espnow_send(const uint8_t *mac_addr, const command_packet_t *cmd) {
    return esp_now_send(mac_addr, (uint8_t *)cmd, sizeof(command_packet_t) + cmd->data_len);
}
