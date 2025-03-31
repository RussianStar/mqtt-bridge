#ifndef ESPNOW_HANDLER_H
#define ESPNOW_HANDLER_H

#include "shared_commands.h"
#include <stdint.h>

typedef void (*espnow_receive_cb_t)(const uint8_t *mac_addr, const command_packet_t *cmd);

void espnow_init(espnow_receive_cb_t receive_cb);
esp_err_t espnow_send(const uint8_t *mac_addr, const command_packet_t *cmd);

#endif /* ESPNOW_HANDLER_H */
