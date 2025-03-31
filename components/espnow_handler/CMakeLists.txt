idf_component_register(
    SRCS "src/espnow_handler.c"
    INCLUDE_DIRS "include"
    REQUIRES shared_commands esp_now
)
