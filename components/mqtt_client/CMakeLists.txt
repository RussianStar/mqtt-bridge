idf_component_register(
    SRCS "src/mqtt_client.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client json shared_commands mqtt esp_event
)
