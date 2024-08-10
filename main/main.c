#include <keypad.h>
#include <stdio.h>
#include <string.h>  // Añadido para usar memset
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <tft.h>
#include <esp_task_wdt.h>
#include "sd_mmc.h"
#include "dtmf.h"

#define BUZZER_PIN 19
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

// Estados del menú
typedef enum {
    ESTADO_BIENVENIDA,
    ESTADO_PRINCIPAL,
    ESTADO_CONFIGURACION,
    ESTADO_REGISTRAR_USUARIO,
    ESTADO_INGRESAR_CEDULA
} EstadoMenu;

// Variables globales
EstadoMenu estado_actual = ESTADO_BIENVENIDA;
int opcion_seleccionada = 1;
char cedula[20] = {0};
int cedula_index = 0;

// Declaración de funciones
void pantalla_task(void *pvParameters);
void teclado_task(void *pvParameters);
int text_length(const char* text);
int16_t get_centered_position(const char* text);
void dibujar_menu_principal();
void dibujar_menu_configuracion();
void dibujar_menu_registrar_usuario();
void dibujar_ingresar_cedula();

void app_main() {
    xTaskCreate(teclado_task, "TECLADO", 4096, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(pantalla_task, "PANTALLA", 4096, NULL, 5, NULL);
}

void teclado_task(void *pvParameters) {
    gpio_num_t keypad[8] = {6, 7, 15, 16, 17, 14, 13, 12};
    
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
                    // No hacemos nada, la pantalla de bienvenida cambiará automáticamente
                    break;
                case ESTADO_PRINCIPAL:
                    if(keypressed == 'C') {
                        estado_actual = ESTADO_CONFIGURACION;
                        opcion_seleccionada = 1;
                    }
                    break;
                case ESTADO_CONFIGURACION:
                    if(keypressed >= '1' && keypressed <= '3') {
                        opcion_seleccionada = keypressed - '0';
                    } else if(keypressed == 'B' && opcion_seleccionada == 1) {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                    }
                    break;
                case ESTADO_REGISTRAR_USUARIO:
                    if(keypressed >= '1' && keypressed <= '4') {
                        opcion_seleccionada = keypressed - '0';
                    } else if(keypressed == 'B' && opcion_seleccionada == 1) {
                        estado_actual = ESTADO_INGRESAR_CEDULA;
                        cedula_index = 0;
                        memset(cedula, 0, sizeof(cedula));
                    }
                    break;
                case ESTADO_INGRESAR_CEDULA:
                    if(keypressed >= '0' && keypressed <= '9' && cedula_index < 19) {
                        cedula[cedula_index++] = keypressed;
                    } else if(keypressed == 'D' && cedula_index > 0) {
                        cedula[--cedula_index] = '\0';
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
    TickType_t last_update = xTaskGetTickCount();

    while (true) {
        if (estado_actual != estado_anterior) {
            TFTfillScreen(ST7735_BLACK);  // Limpiar la pantalla solo cuando el estado cambia

            switch (estado_actual) {
                case ESTADO_BIENVENIDA:
                    TFTdrawText(get_centered_position("BIENVENIDO"), SCREEN_HEIGHT / 2, "BIENVENIDO", ST7735_WHITE, ST7735_BLACK, 1);
                    last_update = xTaskGetTickCount();
                    break;
                case ESTADO_PRINCIPAL:
                    dibujar_menu_principal();
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
            }

            estado_anterior = estado_actual;
        }

        // Cambiar de estado después de 3 segundos en la pantalla de bienvenida
        if (estado_actual == ESTADO_BIENVENIDA && xTaskGetTickCount() - last_update > pdMS_TO_TICKS(3000)) {
            estado_actual = ESTADO_PRINCIPAL;
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Ajustar el retraso según sea necesario
    }
}

void dibujar_menu_principal() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(0, 0, "BIENVENIDO", ST7735_WHITE, ST7735_BLACK, 1);
    char fecha_hora[30];
    snprintf(fecha_hora, sizeof(fecha_hora), "Agosto 10 de 2024 20:00");
    TFTdrawText(get_centered_position(fecha_hora), SCREEN_HEIGHT/2, fecha_hora, ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_menu_configuracion() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(0, 0, "CONFIGURACION", ST7735_WHITE, ST7735_BLACK, 1);
    const char* opciones[] = {"1. REGISTRAR USUARIO", "2. BUSCAR USUARIO", "3. CONF. AVANZADA"};
    for(int i = 0; i < 3; i++) {
        uint16_t color = (i + 1 == opcion_seleccionada) ? ST7735_BLUE : ST7735_BLACK;
        TFTdrawText(0, 30 + i*20, (char*)opciones[i], ST7735_WHITE, color, 1);
    }
}

void dibujar_menu_registrar_usuario() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(0, 0, "REGISTRAR USUARIO", ST7735_WHITE, ST7735_BLACK, 1);
    const char* opciones[] = {"1. CEDULA", "2. HUELLA", "3. PIN", "4. TIPO"};
    for(int i = 0; i < 4; i++) {
        uint16_t color = (i + 1 == opcion_seleccionada) ? ST7735_BLUE : ST7735_BLACK;
        TFTdrawText(0, 30 + i*20, (char*)opciones[i], ST7735_WHITE, color, 1);
    }
}

void dibujar_ingresar_cedula() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(0, 0, "INGRESAR CEDULA", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, 30, cedula, ST7735_WHITE, ST7735_BLACK, 1);
}

int text_length(const char* text) {
    int length = 0;
    while (*text != '\0') {
        length++;
        text++;
    }
    return length;
}

int16_t get_centered_position(const char* text) {
    int text_width = text_length(text) * CHAR_WIDTH;
    return (SCREEN_WIDTH - text_width) / 2;
}
