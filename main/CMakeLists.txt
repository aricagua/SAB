idf_component_register(SRCS "main.c" "tft.c" "keypad.c" "dtmf.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer driver)
