idf_component_register(SRCS "main.c" "tft.c" "keypad.c" "dtmf.c" "as608_esp32_interface.c" "driver_as608.c" "driver_as608_basic.c" "draw.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer driver fatfs sd_card json esp_http_client esp_wifi nvs_flash)
