idf_component_register(
    SRCS "src/custom_mqtt_client.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt esp_event shared_commands
)
