#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

typedef struct {
    char mqtt_uri[128];
    char mqtt_username[32];
    char mqtt_password[32];
    char topic_prefix[32];
} app_config_t;

bool config_init(const char* path);
const app_config_t* config_get();

#endif // CONFIG_MANAGER_H
