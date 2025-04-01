#include "config_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>

#define TAG "CONFIG"

static app_config_t config;

bool config_init(const char* path) {
    if (!path) {
        ESP_LOGE(TAG, "Null config path");
        return false;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open config file: %s", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    if (fsize <= 0) {
        ESP_LOGE(TAG, "Empty config file");
        fclose(f);
        return false;
    }
    fseek(f, 0, SEEK_SET);

    char* json_str = malloc(fsize + 1);
    if (!json_str) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(f);
        return false;
    }

    size_t read = fread(json_str, 1, fsize, f);
    fclose(f);
    if (read != fsize) {
        ESP_LOGE(TAG, "Failed to read config file");
        free(json_str);
        return false;
    }
    json_str[fsize] = 0;

    cJSON* root = cJSON_Parse(json_str);
    free(json_str);
    
    if (!root) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            ESP_LOGE(TAG, "JSON parse error before: %s", error_ptr);
        }
        return false;
    }

    cJSON* mqtt = cJSON_GetObjectItem(root, "mqtt");
    if (mqtt) {
        cJSON* uri = cJSON_GetObjectItem(mqtt, "uri");
        cJSON* user = cJSON_GetObjectItem(mqtt, "username");
        cJSON* pass = cJSON_GetObjectItem(mqtt, "password");
        
        if (uri) strncpy(config.mqtt_uri, uri->valuestring, sizeof(config.mqtt_uri));
        if (user) strncpy(config.mqtt_username, user->valuestring, sizeof(config.mqtt_username));
        if (pass) strncpy(config.mqtt_password, pass->valuestring, sizeof(config.mqtt_password));
    }

    cJSON* topics = cJSON_GetObjectItem(root, "topics");
    if (topics) {
        cJSON* prefix = cJSON_GetObjectItem(topics, "prefix");
        if (prefix) strncpy(config.topic_prefix, prefix->valuestring, sizeof(config.topic_prefix));
    }

    cJSON_Delete(root);
    return true;
}

const app_config_t* config_get() {
    return &config;
}
