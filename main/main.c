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
#include <esp_task_wdt.h>
#include "dtmf.h"
#include "driver_as608_basic.h"
#include <esp_http_client.h>
#include <cJSON.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <stdint.h>
#include <errno.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "dirent.h"
#include "ff.h"
#include "diskio.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "sdkconfig.h"
#include "nvs.h"
#include "common.h"
#include "draw.h"

#define NVS_NAMESPACE "user_data"
// Constants and definitions
#define FIREBASE_HOST "https://users-89d5a-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "your-database-secret"
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
#define MOUNT_POINT "/sdcard"
#define MAX_USER_DATA_SIZE 256

#define MAX_RETRY 3
#define LOG_FILE_PATH MOUNT_POINT"/log.txt"

// Pin assignments for SD card
#define PIN_NUM_MISO  37
#define PIN_NUM_MOSI  35
#define PIN_NUM_CLK   36
#define PIN_NUM_CS    38

static const char *TAG = "main";

sdmmc_card_t *card;


// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


// Global variables
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

void wifi_init_sta();

void print_nvs_stats();


static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

esp_err_t delete_fingerprint(uint16_t page_number);
esp_err_t register_fingerprint(uint16_t *page_number); 
esp_err_t mount_sdcard(void);
esp_err_t save_user_data(const char *mount_point, const DatosUsuario *usuario);
esp_err_t load_user_data_txt(DatosUsuario *usuario, const char *cedula);
esp_err_t delete_all_fingerprints(void);
esp_err_t format_sd_card(const char *mount_point);
esp_err_t register_attendance(uint16_t page_number);
void estado_asistencia_task(void *pvParameters);

const char* data = "Callback function called";
/*************************sd************************************/
void app_main() {
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    if (mount_sdcard() != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount SD card");

    }

    // Initialize WiFi
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    


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
                                    estado_actual = ESTADO_ASISTENCIA;
                                    break;
                                case 5:
                                    estado_actual = ESTADO_RESETEAR_SISTEMA;
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
                        esp_err_t sd_result = format_sd_card(MOUNT_POINT);
                        if (fingerprint_result == ESP_OK && sd_result == ESP_OK) {
                            estado_actual = ESTADO_RESET_EXITOSO;
                        } else {
                            estado_actual = ESTADO_ERROR_RESET;
                        }
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_CONFIGURACION;
                        opcion_seleccionada = 1;
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
                            esp_err_t ret = save_user_data(MOUNT_POINT, &usuario_actual);
                            if (ret == ESP_OK) {
                                estado_actual = ESTADO_REGISTRO_EXITOSO;
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
                    }
                    break;
                case ESTADO_ASISTENCIA:
    		    if(keypressed == 'B') {
                    estado_actual = ESTADO_CONFIGURACION;
                    opcion_seleccionada = 1;
                    } else if (keypressed == 'A') {
                    iniciar_estado_asistencia();
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

    while (true) {
        if (estado_actual != estado_anterior || opcion_seleccionada != opcion_seleccionada_anterior) {
            TFTfillScreen(ST7735_BLACK);

            switch (estado_actual) {
                case ESTADO_BIENVENIDA:
                    TFTdrawText(get_centered_position("BIENVENIDO"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT / 2, "BIENVENIDO", ST7735_WHITE, ST7735_BLACK, 1);
                    last_update = xTaskGetTickCount();
                    break;
                case ESTADO_PRINCIPAL:
                    dibujar_menu_principal();
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


esp_err_t mount_sdcard(void)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 1000;  // Set the SPI frequency to 10 MHz (10,000 kHz)

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount filesystem");
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    return ESP_OK;
}


esp_err_t save_user_data(const char *mount_point, const DatosUsuario *usuario)
{

    char file_path[64];
    
    // Format the file path using huella_pagina as an integer
    snprintf(file_path, sizeof(file_path), "%s/user_%d.txt", mount_point, usuario->huella_pagina);
    
    
    ESP_LOGI(TAG, "Opening file %s", file_path);
    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(usuario, sizeof(DatosUsuario), 1, f);
    fclose(f);
    
    if (written != 1) {
        ESP_LOGE(TAG, "Failed to write struct to file");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "File written successfully");
    return ESP_OK;
}
esp_err_t load_user_data_txt(DatosUsuario *usuario, const char *cedula)
{
    char file_path[64];
    snprintf(file_path, sizeof(file_path), MOUNT_POINT"/user_%s.txt", cedula);
	
    ESP_LOGI(TAG, "Opening file %s", file_path);
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Cedula: %19s", usuario->cedula) == 1) continue;
        if (sscanf(line, "Huella Registrada: %3s", line) == 1) {
            usuario->huella_registrada = (strcmp(line, "Si") == 0);
            continue;
        }
        if (sscanf(line, "Huella Pagina: %hu", &usuario->huella_pagina) == 1) continue;
        if (sscanf(line, "PIN: %4s", usuario->pin) == 1) continue;
        if (sscanf(line, "Tipo: %19s", usuario->tipo) == 1) continue;
    }

    fclose(f);
    ESP_LOGI(TAG, "File read");
    return ESP_OK;
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
	
esp_err_t format_sd_card(const char *mount_point)
{
    esp_err_t ret;

    // Check if the card is mounted
    struct stat st;
    if (stat(mount_point, &st) != 0) {
        ESP_LOGE(TAG, "SD card is not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    // Perform the formatting
    ESP_LOGI(TAG, "Formatting SD card...");
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format SD card (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD card formatted successfully");

    // Verify that the card is empty
    DIR *dir = opendir(mount_point);
    if (dir) {
        struct dirent *ent;
        bool is_empty = true;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                is_empty = false;
                break;
            }
        }
        closedir(dir);
        if (!is_empty) {
            ESP_LOGW(TAG, "SD card was formatted but still contains files");
        }
    } else {
        ESP_LOGW(TAG, "Unable to open formatted SD card directory");
    }

    return ESP_OK;
}

esp_err_t register_attendance(uint16_t page_number) {
    char user_file[64];
    snprintf(user_file, sizeof(user_file), MOUNT_POINT"/user_%d.txt", page_number);
    
    ESP_LOGI(TAG, "Opening user file %s", user_file);
    FILE *f = fopen(user_file, "r");  
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open user file for reading");
        return ESP_FAIL;
    }
    
    DatosUsuario usuario;
    size_t read = fread(&usuario, sizeof(DatosUsuario), 1, f);
    fclose(f);
    
    if (read != 1) {
        ESP_LOGE(TAG, "Failed to read user data");
        return ESP_FAIL;
    }

    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Opening log file %s", LOG_FILE_PATH);
    FILE *log_file = fopen(LOG_FILE_PATH, "a");  // Open in append mode
    if (log_file == NULL) {
       ESP_LOGE(TAG, "Failed to open log file for writing: %s", strerror(errno));
       return ESP_FAIL;
    }
    
    fprintf(log_file, "%04d-%02d-%02d %02d:%02d:%02d, %s, %s\n",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
            usuario.cedula, usuario.tipo);
    
    fclose(log_file);
    
    ESP_LOGI(TAG, "Attendance registered for user: %s", usuario.cedula);
    return ESP_OK;
}
void estado_asistencia_task(void *pvParameters) {
    uint8_t res;
    uint16_t score;
    uint16_t found_page;  // Correctly declare found_page
    as608_status_t status;
    int retry_count = 0;

    while (estado_actual == ESTADO_ASISTENCIA) {
        ESP_LOGI(TAG, "Place your finger on the sensor");
        dibujar_esperando_huella();  // Function to update the display

        res = as608_basic_verify(&found_page, &score, &status);

        if (res == 0) {
            if (status == AS608_STATUS_OK) {
                ESP_LOGI(TAG, "Fingerprint matched. Page: %d, Score: %d", found_page, score);
                if (register_attendance(found_page) == ESP_OK) {
                    dibujar_asistencia_registrada(found_page);  // Function to update the display
                    vTaskDelay(pdMS_TO_TICKS(2000));  // Display success message for 2 seconds
                } else {
                    dibujar_error_registro_asistencia();  // Function to update the display
                    vTaskDelay(pdMS_TO_TICKS(2000));  // Display error message for 2 seconds
                }
                retry_count = 0;
            } else {
                ESP_LOGI(TAG, "Fingerprint not recognized. Status: %d", status);
                dibujar_huella_no_reconocida();  // Function to update the display
                vTaskDelay(pdMS_TO_TICKS(2000));  // Display error message for 2 seconds
                retry_count++;
            }
        } else {
            ESP_LOGE(TAG, "Error in fingerprint verification. Result: %d", res);
            dibujar_error_verificacion();  // Function to display verification error
            vTaskDelay(pdMS_TO_TICKS(2000));  // Display error message for 2 seconds
            retry_count++;
        }

        if (retry_count >= MAX_RETRY) {
            ESP_LOGI(TAG, "Max retries reached. Returning to main menu.");
            estado_actual = ESTADO_PRINCIPAL;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Small delay to prevent tight looping
    }

    vTaskDelete(NULL);
}
