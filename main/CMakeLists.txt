idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES shared_commands espnow_handler mqtt_client config_manager
)
