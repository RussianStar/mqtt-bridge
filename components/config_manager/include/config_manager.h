#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string.h>
#include <stdbool.h>

typedef struct {
    char mqtt_uri[128];
    char mqtt_username[64];
    char mqtt_password[64];
    char topic_prefix[32];
} app_config_t;

bool config_manager_init(const char *config_path);
const app_config_t* config_manager_get(void);

#endif // CONFIG_MANAGER_H
