#include <keypad.h>
#include <stdio.h>
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
#define CHAR_WIDTH 6

// Declaración de funciones
void pantalla_task(void *pvParameters);
void teclado_task(void *pvParameters);
int text_length(const char* text);
int16_t get_centered_position(const char* text);

void app_main() {
    // Crear tarea para el teclado
    xTaskCreate(teclado_task, "TECLADO", 4096, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    // Crear tarea para la pantalla
    xTaskCreate(pantalla_task, "PANTALLA", 4096, NULL, 5, NULL);
}

void teclado_task(void *pvParameters) {
    // Configuración de pines del teclado
    gpio_num_t keypad[8] = {6, 7, 15, 16, 17, 14, 13, 12};

    // Inicializar el teclado
    esp_err_t init_result = keypad_initalize(keypad);
    if (init_result == ESP_OK) {
        ESP_LOGI("KEYPAD", "El teclado se inicializó correctamente");
    } else {
        ESP_LOGE("KEYPAD", "Algo falló al inicializar el teclado");
        vTaskDelete(NULL);
    }

    // Inicializar DTMF
    init_result = dtmf_init(BUZZER_PIN);
    if (init_result == ESP_OK) {
        ESP_LOGI("DTMF", "DTMF Funciona");
    } else {
        ESP_LOGE("DTMF", "Algo falló con el DTMF");
        vTaskDelete(NULL);
    }

    // Bucle principal de la tarea del teclado
    while(true) {
        char keypressed = keypad_getkey();  // Obtener tecla presionada
        if(keypressed != '\0') {
            ESP_LOGI("KEYPAD", "Pressed key: %c", keypressed);
            dtmf_play_tone(keypressed);
        } else {
            ESP_LOGV("KEYPAD", "No key pressed");
        }
    	// Dividir la tarea en partes más pequeñas
    	for (int i = 0; i < 10; i++) {
        vTaskDelay(pdMS_TO_TICKS(10)); // Añadir un pequeño retardo para evitar el uso excesivo de CPU
    }
    }
}

void pantalla_task(void *pvParameters) {
    // Inicializar TFT
    TFT_Initialize(20, 21, 48, 45, 47); // Ajustar estos pines según tu conexión
    TFTfillScreen(ST7735_BLACK);

    // Configurar propiedades del texto
    TFTFontNum(TFTFont_Default);
    TFTsetTextWrap(true);

    // Calcular posición centrada del texto
    const char* line1 = "BIENVENIDO ADMIN";
    const char* line2 = "CONFIGURE UN PIN";
    int16_t x1 = get_centered_position(line1);
    int16_t x2 = get_centered_position(line2);

    // Dibujar texto centrado
    TFTdrawText(x1, 50, (char*)line1, ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(x2, 70, (char*)line2, ST7735_WHITE, ST7735_BLACK, 1);

    // Evitar que la tarea termine
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Función para calcular la longitud del texto
int text_length(const char* text) {
    int length = 0;
    while (*text != '\0') {
        length++;
        text++;
    }
    return length;
}

// Función para calcular la posición centrada del texto
int16_t get_centered_position(const char* text) {
    int text_width = text_length(text) * CHAR_WIDTH;
    return (SCREEN_WIDTH - text_width) / 2;
}

