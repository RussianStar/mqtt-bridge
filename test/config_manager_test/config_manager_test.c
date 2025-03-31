#include "unity.h"
#include "config_manager.h"
#include "esp_log.h"
#include <stdio.h>

#define TEST_CONFIG_PATH "test_config.json"

void setUp(void) {
    // Create test config file
    FILE* f = fopen(TEST_CONFIG_PATH, "w");
    fprintf(f, "{\n");
    fprintf(f, "    \"mqtt\": {\n");
    fprintf(f, "        \"uri\": \"mqtt://test.broker\",\n");
    fprintf(f, "        \"username\": \"testuser\",\n");
    fprintf(f, "        \"password\": \"testpass\"\n");
    fprintf(f, "    },\n");
    fprintf(f, "    \"topics\": {\n");
    fprintf(f, "        \"prefix\": \"test_prefix\"\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n");
    fclose(f);
}

void tearDown(void) {
    remove(TEST_CONFIG_PATH);
}

void test_config_init_valid(void) {
    TEST_ASSERT_TRUE(config_init(TEST_CONFIG_PATH));
    
    const app_config_t* cfg = config_get();
    TEST_ASSERT_EQUAL_STRING("mqtt://test.broker", cfg->mqtt_uri);
    TEST_ASSERT_EQUAL_STRING("testuser", cfg->mqtt_username);
    TEST_ASSERT_EQUAL_STRING("testpass", cfg->mqtt_password);
    TEST_ASSERT_EQUAL_STRING("test_prefix", cfg->topic_prefix);
}

void test_config_init_invalid_path(void) {
    TEST_ASSERT_FALSE(config_init("nonexistent.json"));
}

void test_config_init_empty_file(void) {
    FILE* f = fopen("empty.json", "w");
    fclose(f);
    TEST_ASSERT_FALSE(config_init("empty.json"));
    remove("empty.json");
}

void test_config_init_malformed_json(void) {
    FILE* f = fopen("bad.json", "w");
    fprintf(f, "{ invalid json }");
    fclose(f);
    TEST_ASSERT_FALSE(config_init("bad.json"));
    remove("bad.json");
}

void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_init_valid);
    RUN_TEST(test_config_init_invalid_path);
    RUN_TEST(test_config_init_empty_file);
    RUN_TEST(test_config_init_malformed_json);
    UNITY_END();
}
