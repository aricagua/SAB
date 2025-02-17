#include <keypad.h>
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <tft.h>
#include "dtmf.h"
#include "driver_as608_basic.h"
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"
#include "nvs_user_data_module.h"
#include "common.h"
#include "draw.h"
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "sntp_time_module.h"


#define MAX_USERS 100
#define NORMAL_FONT_SCALE 2
#define WIFI_SSID "El Guerrero"
#define WIFI_PASS "Waleska5500"
#define MAX_PAGE_DIGITS 3
#define BUZZER_PIN 19
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define ADMIN_PIN "3141"
#define MAX_CEDULA_LENGTH 20
#define MAX_TYPE_LENGTH 20
#define PIN_LENGTH 4
#define MAX_USER_DATA_SIZE 256

#define MAX_RETRY 3
static const char *TAG = "main";
static const char *TAG_TIME = "time_sync";


// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


// Global variables
// Global variable to store the current time
struct tm timeinfo;
// In main.c
EstadoMenu estado_actual = ESTADO_BIENVENIDA;
DatosUsuario usuario_actual = {0};
char admin_pin_input[5] = {0};
uint16_t page_to_delete = 0;
bool as608_initialized = false;
int input_index = 0;


// Function prototypes
void pantalla_task(void *pvParameters);
void teclado_task(void *pvParameters);
void sync_time(void);
void initialize_sntp(void);
void get_current_time(struct tm *timeinfo);

void wifi_init_sta();

void print_nvs_stats(); // Function definition not found in provided code, assuming it's unused and can be removed if confirmed.



static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

esp_err_t delete_fingerprint(uint16_t page_number);
esp_err_t register_fingerprint(uint16_t *page_number);


esp_err_t delete_all_fingerprints(void);

void estado_asistencia_task(void *pvParameters);


const char* data = "Callback function called";
void app_main() {

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);



    // Initialize WiFi
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // Sync time
    sync_time();

    // Initialize AS608 fingerprint sensor
    uint32_t addr = 0xFFFFFFFF;  // Default address
    uint8_t res = as608_basic_init(addr);
    if (res != 0) {
        ESP_LOGE(TAG, "AS608 initialization failed");
    } else {
        ESP_LOGI(TAG, "AS608 initialized successfully");
        as608_initialized = true;
    }

    // Create tasks
    xTaskCreate(teclado_task, "TECLADO", 4096, NULL, 5, NULL);
    xTaskCreate(pantalla_task, "PANTALLA", 4096, NULL, 5, NULL);
}

void iniciar_estado_asistencia() {
    xTaskCreate(estado_asistencia_task, "ASISTENCIA_TASK", 4096, NULL, 5, NULL);
}

void teclado_task(void *pvParameters) {
    gpio_num_t keypad[8] = {6, 7, 15, 16, 8, 14, 13, 12};

    esp_err_t init_result = keypad_initalize(keypad);
    if (init_result != ESP_OK) {
        ESP_LOGE("KEYPAD", "Fallo al inicializar el teclado");
        vTaskDelete(NULL);
    }

    init_result = dtmf_init(BUZZER_PIN);
    if (init_result != ESP_OK) {
        ESP_LOGE("DTMF", "Fallo al inicializar DTMF");
        vTaskDelete(NULL);
    }

    TickType_t last_fingerprint_check = 0;
    const TickType_t fingerprint_check_interval = pdMS_TO_TICKS(3000);
    static uint32_t log_start_index = 0;

    while(true) {
        char keypressed = keypad_getkey();
        if(keypressed != '\0') {
            ESP_LOGI("KEYPAD", "Tecla presionada: %c", keypressed);
            dtmf_play_tone(keypressed);

            switch(estado_actual) {
               case ESTADO_BIENVENIDA:
                    estado_actual = ESTADO_PRINCIPAL;
                    break;
                case ESTADO_PRINCIPAL:
                    if(keypressed == 'C') {
                        estado_actual = ESTADO_INGRESAR_PIN_ADMIN;
                        input_index = 0;
                        memset(admin_pin_input, 0, sizeof(admin_pin_input));
                    } else if (keypressed == '*'){
                        estado_actual = ESTADO_ASISTENCIA;
                    }
                    break;
                case ESTADO_ASISTENCIA:
         if (estado_actual == ESTADO_ASISTENCIA) {
            TickType_t now = xTaskGetTickCount();
            if (now - last_fingerprint_check >= fingerprint_check_interval) {
                last_fingerprint_check = now;
                dibujar_esperando_huella();
                ESP_LOGI(TAG, "Scanning for fingerprint...");

                uint8_t res;
                uint16_t score, found_page;
                as608_status_t status;

                res = as608_basic_verify(&found_page, &score, &status);

                if (res == 0 && status == AS608_STATUS_OK) {
                    DatosUsuario usuario;
                    if (load_user_data_nvs(&usuario, found_page) == ESP_OK &&
                        register_attendance_nvs(&usuario) == ESP_OK) {
                        dibujar_asistencia_registrada(found_page);
                        ESP_LOGI(TAG, "Attendance registered for user: %s", usuario.cedula);
                        vTaskDelay(pdMS_TO_TICKS(3000));
                        estado_actual = ESTADO_PRINCIPAL;
                    } else {
                        dibujar_error_registro_asistencia();
                        ESP_LOGE(TAG, "Error registering attendance");
                        vTaskDelay(pdMS_TO_TICKS(3000));
                    }
                } else {
                    dibujar_huella_no_reconocida();
                    ESP_LOGI(TAG, "Fingerprint not recognized");
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Show "not recognized" for 1 second
                }
            }
            }
            break;
                case ESTADO_INGRESAR_PIN_ADMIN:
                    if(keypressed >= '0' && keypressed <= '9' && input_index < 4) {
                        admin_pin_input[input_index++] = keypressed;
                        admin_pin_input[input_index] = '\0';
                    } else if(keypressed == 'D' && input_index > 0) {
                        admin_pin_input[--input_index] = '\0';
                    } else if(keypressed == 'A') {
                        if(strcmp(admin_pin_input, ADMIN_PIN) == 0) {
                            estado_actual = ESTADO_CONFIGURACION;
                            opcion_seleccionada = 1;
                        } else {
                            estado_actual = ESTADO_PRINCIPAL;
                        }
                        memset(admin_pin_input, 0, sizeof(admin_pin_input));
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_PRINCIPAL;
                        memset(admin_pin_input, 0, sizeof(admin_pin_input));
                    }
                    break;
                    case ESTADO_CONFIGURACION:
                        if(keypressed >= '1' && keypressed <= '5') {
                            opcion_seleccionada = keypressed - '0';
                        } else if(keypressed == 'A') {
                            switch(opcion_seleccionada) {
                                case 1:
                                    estado_actual = ESTADO_REGISTRAR_USUARIO;
                                    opcion_seleccionada = 1;
                                    memset(&usuario_actual, 0, sizeof(DatosUsuario));
                                    break;
                                case 2:
                                    // Implement user search functionality
                                    break;
                                case 3:
                                    // Implement advanced configuration
                                    break;
                                case 4:
                                    estado_actual = ESTADO_RESETEAR_SISTEMA;
                                    break;
                                case 5:
                                    estado_actual = ESTADO_VER_REGISTRO;
                                    log_start_index = 0;
                                    break;
                            }
                        } else if(keypressed == 'B') {
                            estado_actual = ESTADO_PRINCIPAL;
                            opcion_seleccionada = 1;
                        }
                        break;
                // Add a new case for ESTADO_RESETEAR_SISTEMA
                case ESTADO_RESETEAR_SISTEMA:
                    if(keypressed == 'A') {
                        esp_err_t fingerprint_result = delete_all_fingerprints();
                        esp_err_t nvs_result = erase_all_user_data_nvs();
                        if (fingerprint_result == ESP_OK && nvs_result == ESP_OK) {
                            estado_actual = ESTADO_RESET_EXITOSO;
                        } else {
                            estado_actual = ESTADO_ERROR_RESET;
                        }
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_CONFIGURACION;
                        opcion_seleccionada = 1;
                    }
                    break;
case ESTADO_VER_REGISTRO:
    if(keypressed == 'B') {
        ESP_LOGI(TAG, "B key pressed in ESTADO_VER_REGISTRO");
        if (log_start_index >= LOGS_PER_PAGE) {
            log_start_index -= LOGS_PER_PAGE;
            ESP_LOGI(TAG, "Moving to previous page. New start index: %lu", (unsigned long)log_start_index);
        } else {
            estado_actual = ESTADO_CONFIGURACION;
            opcion_seleccionada = 5;
            ESP_LOGI(TAG, "Returning to ESTADO_CONFIGURACION");
        }
    } else if(keypressed == 'A') {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open("attendance_log", NVS_READONLY, &nvs_handle);
        if (err == ESP_OK) {
            uint32_t log_count = 0;
            nvs_get_u32(nvs_handle, "log_count", &log_count);
            nvs_close(nvs_handle);

            if (log_start_index + LOGS_PER_PAGE < log_count) {
                log_start_index += LOGS_PER_PAGE;
                ESP_LOGI(TAG, "Moving to next page. New start index: %lu", (unsigned long)log_start_index);
            }
        }
    } else if(keypressed == 'C') {
        estado_actual = ESTADO_CONFIGURACION;
        opcion_seleccionada = 5;
        ESP_LOGI(TAG, "Returning to ESTADO_CONFIGURACION");
    }
    break;

                case ESTADO_RESET_EXITOSO:
                case ESTADO_ERROR_RESET:
                    if(keypressed == 'A') {
                        estado_actual = ESTADO_CONFIGURACION;
                        opcion_seleccionada = 1;
                    }
                    break;
                case ESTADO_REGISTRAR_USUARIO:
                    if(keypressed >= '1' && keypressed <= '4') {
                        opcion_seleccionada = keypressed - '0';
                    } else if(keypressed == 'A') {
                        switch(opcion_seleccionada) {
                            case 1:
                                estado_actual = ESTADO_INGRESAR_CEDULA;
                                input_index = 0;
                                break;
                            case 2:
                                estado_actual = ESTADO_INGRESAR_HUELLA;
                                break;
                            case 3:
                                estado_actual = ESTADO_INGRESAR_PIN;
                                input_index = 0;
                                break;
                            case 4:
                                estado_actual = ESTADO_SELECCIONAR_TIPO;
                                break;
                        }
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_CONFIGURACION;
                        opcion_seleccionada = 1;
                    } else  if(keypressed == 'C') {
                        if(strlen(usuario_actual.cedula) > 0 && usuario_actual.huella_registrada &&
                           strlen(usuario_actual.pin) > 0 && strlen(usuario_actual.tipo) > 0) {
                            esp_err_t ret = save_user_data_nvs(&usuario_actual);
                            if (ret == ESP_OK) {
                                estado_actual = ESTADO_REGISTRO_EXITOSO;
                		memset(&usuario_actual, 0, sizeof(DatosUsuario));
                		input_index = 0;
                            } else {
                                estado_actual = ESTADO_ERROR_REGISTRO;
                            }
                        } else {
                            estado_actual = ESTADO_ERROR_REGISTRO;
                        }
                    }
                    break;
                case ESTADO_INGRESAR_CEDULA:
                    if(keypressed >= '0' && keypressed <= '9' && input_index < 19) {
                        usuario_actual.cedula[input_index++] = keypressed;
                        usuario_actual.cedula[input_index] = '\0';
                    } else if(keypressed == 'D' && input_index > 0) {
                        usuario_actual.cedula[--input_index] = '\0';
                    } else if(keypressed == 'A' || keypressed == 'B') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                    }
                    break;
                case ESTADO_INGRESAR_HUELLA:
                    if(keypressed == 'A') {
                        if (as608_initialized) {
                            uint16_t page_number;
                            esp_err_t ret = register_fingerprint(&page_number);
                            if (ret == ESP_OK) {
                                usuario_actual.huella_registrada = true;
                                usuario_actual.huella_pagina = page_number;
                                estado_actual = ESTADO_REGISTRAR_USUARIO;
                                opcion_seleccionada = 1;
                            } else {
                                // Handle error (you might want to display an error message)
                            }
                        } else {
                            // Handle case where AS608 is not initialized
                        }
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    }
                    break;

                case ESTADO_INGRESAR_PIN:
                    if(keypressed >= '0' && keypressed <= '9' && input_index < 4) {
                        usuario_actual.pin[input_index++] = keypressed;
                        usuario_actual.pin[input_index] = '\0';
                    } else if(keypressed == 'D' && input_index > 0) {
                        usuario_actual.pin[--input_index] = '\0';
                    } else if(keypressed == 'A' && input_index == 4) {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    }
                    break;
                case ESTADO_SELECCIONAR_TIPO:
                    if(keypressed == '1') {
                        strcpy(usuario_actual.tipo, "ADMINISTRADOR");
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    } else if(keypressed == '2') {
                        strcpy(usuario_actual.tipo, "USUARIO");
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    }
                    break;
                case ESTADO_ERROR_REGISTRO:
                case ESTADO_REGISTRO_EXITOSO:
                    if(keypressed == 'A') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
        		memset(&usuario_actual, 0, sizeof(DatosUsuario));
        		input_index = 0;
                    }
                    break;
                case ESTADO_RESUMEN_REGISTRO:
                    if(keypressed == 'A') {
                        // Implement logic to save the user data
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                    }
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void pantalla_task(void *pvParameters) {
    TFT_Initialize(20, 21, 48, 45, 47);
    TFTfillScreen(ST7735_BLACK);
    TFTFontNum(TFTFont_Default);
    TFTsetTextWrap(true);

    EstadoMenu estado_anterior = ESTADO_BIENVENIDA;
    int opcion_seleccionada_anterior = 0;
    TickType_t last_update = xTaskGetTickCount();
    TickType_t last_time_update = 0;
    static uint32_t log_start_index = 0;

    while (true) {
        TickType_t now = xTaskGetTickCount();

        if (estado_actual != estado_anterior || opcion_seleccionada != opcion_seleccionada_anterior ||
            (estado_actual == ESTADO_PRINCIPAL && now - last_time_update > pdMS_TO_TICKS(60000))) { // Update every minute
            TFTfillScreen(ST7735_BLACK);

            switch (estado_actual) {
                case ESTADO_BIENVENIDA:
                    TFTdrawText(get_centered_position("BIENVENIDO"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT / 2, "BIENVENIDO", ST7735_WHITE, ST7735_BLACK, 1);
                    last_update = xTaskGetTickCount();
                    break;
                case ESTADO_PRINCIPAL:
                    dibujar_menu_principal();
                    last_time_update = now;
                    break;
                case ESTADO_INGRESAR_PIN_ADMIN:
                    dibujar_ingresar_pin_admin();
                    break;
                case ESTADO_CONFIGURACION:
                    dibujar_menu_configuracion();
                    break;
                case ESTADO_REGISTRAR_USUARIO:
                    dibujar_menu_registrar_usuario();
                    break;
                case ESTADO_INGRESAR_CEDULA:
                    dibujar_ingresar_cedula();
                    break;
                case ESTADO_INGRESAR_HUELLA:
                    dibujar_ingresar_huella();
                    break;
                case ESTADO_INGRESAR_PIN:
                    dibujar_ingresar_pin();
                    break;
                case ESTADO_SELECCIONAR_TIPO:
                    dibujar_seleccionar_tipo();
                    break;
                case ESTADO_RESUMEN_REGISTRO:
                    dibujar_resumen_registro();
                    break;
                case ESTADO_ASISTENCIA:
                    dibujar_esperando_huella();
                    break;
                case ESTADO_ERROR_REGISTRO:
                    dibujar_error_registro();
                    break;
                case ESTADO_REGISTRO_EXITOSO:
                    dibujar_registro_exitoso();
                    break;
                case ESTADO_RESETEAR_SISTEMA:
                    dibujar_resetear_sistema();
                    break;
                case ESTADO_RESET_EXITOSO:
                    dibujar_reset_exitoso();
                    break;
                case ESTADO_ERROR_RESET:
                    dibujar_error_reset();
                    break;
                case ESTADO_VER_REGISTRO:
                    dibujar_ver_registro(log_start_index);
                    break;
                default:
                    ESP_LOGE("PANTALLA", "Estado no manejado: %d", estado_actual);
                    break;
            }

            estado_anterior = estado_actual;
            opcion_seleccionada_anterior = opcion_seleccionada;
        }

        if (estado_actual == ESTADO_BIENVENIDA && xTaskGetTickCount() - last_update > pdMS_TO_TICKS(3000)) {
            estado_actual = ESTADO_PRINCIPAL;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t delete_fingerprint(uint16_t page_number)
{
    uint8_t res;
    as608_status_t status;

    ESP_LOGI("FINGERPRINT", "Attempting to delete fingerprint at page %d", page_number);

    // Attempt to delete the fingerprint
    res = as608_basic_delete_fingerprint(page_number, &status);

    if (res != 0) {
        ESP_LOGE("FINGERPRINT", "Failed to delete fingerprint. Error code: %d", res);
        return ESP_FAIL;
    }

    if (status != AS608_STATUS_OK) {
        ESP_LOGE("FINGERPRINT", "Fingerprint deletion failed. Status: %d", status);
        return ESP_FAIL;
    }

    ESP_LOGI("FINGERPRINT", "Fingerprint deleted successfully. Page number: %d", page_number);

    return ESP_OK;
}


esp_err_t register_fingerprint(uint16_t *page_number)
{
    uint8_t res;
    uint16_t score;
    as608_status_t status;

    // Callback function to handle fingerprint registration process
    void fingerprint_callback(int8_t cb_status, const char *const fmt, ...) {
        char buffer[100];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        // You might want to display this information on the TFT screen
        ESP_LOGI("FINGERPRINT", "%s", buffer);
    }

    // Attempt to register the fingerprint
    res = as608_basic_input_fingerprint(fingerprint_callback, &score, page_number, &status);

    if (res != 0) {
        ESP_LOGE("FINGERPRINT", "Failed to register fingerprint. Error code: %d", res);
        return ESP_FAIL;
    }

    if (status != AS608_STATUS_OK) {
        ESP_LOGE("FINGERPRINT", "Fingerprint registration failed. Status: %d", status);
        return ESP_FAIL;
    }

    ESP_LOGI("FINGERPRINT", "Fingerprint registered successfully. Page number: %d, Score: %d", *page_number, score);

    return ESP_OK;
}


static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Connection to the AP failed. Retrying.");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}


esp_err_t delete_all_fingerprints(void)
{
    uint8_t res;
    as608_status_t status;

    ESP_LOGI("FINGERPRINT", "Attempting to delete all fingerprints");

    // Use the AS608 function to delete all fingerprints at once
    res = as608_basic_empty_fingerprint(&status);

    if (res != 0) {
        ESP_LOGE("FINGERPRINT", "Failed to delete all fingerprints. Error code: %d", res);
        return ESP_FAIL;
    }

    if (status != AS608_STATUS_OK) {
        ESP_LOGE("FINGERPRINT", "Fingerprint deletion failed. Status: %d", status);
        return ESP_FAIL;
    }

    ESP_LOGI("FINGERPRINT", "All fingerprints deleted successfully");
    return ESP_OK;
}

void estado_asistencia_task(void *pvParameters) {
    while (true) {
        if (estado_actual == ESTADO_ASISTENCIA) {
            dibujar_esperando_huella();
            vTaskDelay(pdMS_TO_TICKS(3000));

            uint8_t res;
            uint16_t score, found_page;
            as608_status_t status;

            res = as608_basic_verify(&found_page, &score, &status);

            if (res == 0 && status == AS608_STATUS_OK) {
                DatosUsuario usuario;
                if (load_user_data_nvs(&usuario, found_page) == ESP_OK &&
                    register_attendance_nvs(&usuario) == ESP_OK) {
                    dibujar_asistencia_registrada(found_page);
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    estado_actual = ESTADO_PRINCIPAL;
                } else {
                    dibujar_error_registro_asistencia();
                }
            } else {
                dibujar_huella_no_reconocida();
            }

            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}