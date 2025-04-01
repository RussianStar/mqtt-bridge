#include "custom_mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_event.h"
#include <string.h>
#include <inttypes.h>

#define TAG "MQTT"

static esp_mqtt_client_handle_t client;
static char topic_prefix[64];
static mqtt_command_cb_t command_callback;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/**
 * @brief Event handler registered to receive MQTT events
 *
 * This function is called by the MQTT client event loop.
 *
 * @param handler_args User data registered to the event.
 * @param base Event base for the handler (always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t event_client = event->client;
    int msg_id;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            
            // Subscribe to commands topic
            char subscribe_topic[128];
            snprintf(subscribe_topic, sizeof(subscribe_topic), "%s/+/commands/#", topic_prefix);
            msg_id = esp_mqtt_client_subscribe(event_client, subscribe_topic, 1);
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            
            if (command_callback) {
                // Extract topic and data
                char topic[256];
                strncpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';
                
                char data[256];
                strncpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';
                
                // Extract MAC from topic
                // Format: <prefix>/<mac>/commands/<command>
                char *mac_start = strstr(topic, topic_prefix);
                if (mac_start) {
                    mac_start += strlen(topic_prefix) + 1; // Skip prefix and '/'
                    char *mac_end = strchr(mac_start, '/');
                    if (mac_end) {
                        char mac_str[18];
                        size_t mac_len = mac_end - mac_start;
                        strncpy(mac_str, mac_start, mac_len);
                        mac_str[mac_len] = '\0';
                        
                        // Extract command
                        char *cmd_start = strstr(topic, "commands/");
                        if (cmd_start) {
                            cmd_start += 9; // length of "commands/"
                            command_callback(mac_str, cmd_start, data);
                        }
                    }
                }
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
            
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

esp_err_t mqtt_init(mqtt_command_cb_t command_cb, 
              const mqtt_client_config_t* config,
              const char* prefix) {
    command_callback = command_cb;
    strncpy(topic_prefix, prefix, sizeof(topic_prefix)-1);
    topic_prefix[sizeof(topic_prefix)-1] = '\0';
    
    // Configure MQTT client
    esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.broker.address.uri = config->uri;
    mqtt_cfg.credentials.username = config->username;
    mqtt_cfg.credentials.authentication.password = config->password;
    
    // Initialize MQTT client
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        return ESP_FAIL;
    }
    
    // Register event handler
    esp_err_t err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }
    
    // Start MQTT client
    return esp_mqtt_client_start(client);
}

esp_err_t mqtt_publish_status(const char* mac_str, const char* command, const char* payload) {
    if (client == NULL) {
        return ESP_FAIL;
    }
    
    char topic[256];
    snprintf(topic, sizeof(topic), "%s/%s/status/%s/data", topic_prefix, mac_str, command);
    
    int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Published to %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

void mqtt_publish_mac_address(const uint8_t mac[6]) {
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/bridge/mac", topic_prefix);

    int msg_id = esp_mqtt_client_publish(client, topic, mac_str, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish MAC address");
    } else {
        ESP_LOGI(TAG, "Published MAC address: %s, msg_id=%d", mac_str, msg_id);
    }
}
