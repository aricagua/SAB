#include <keypad.h>
#include <stdio.h>
#include <string.h>
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

typedef enum {
    ESTADO_BIENVENIDA,
    ESTADO_PRINCIPAL,
    ESTADO_CONFIGURACION,
    ESTADO_REGISTRAR_USUARIO,
    ESTADO_INGRESAR_CEDULA,
    ESTADO_INGRESAR_HUELLA,
    ESTADO_INGRESAR_PIN,
    ESTADO_SELECCIONAR_TIPO,
    ESTADO_RESUMEN_REGISTRO
} EstadoMenu;

typedef struct {
    char cedula[20];
    bool huella_registrada;
    char pin[5];
    char tipo[20];
} DatosUsuario;

EstadoMenu estado_actual = ESTADO_BIENVENIDA;
int opcion_seleccionada = 1;
DatosUsuario usuario_actual = {0};
int input_index = 0;

void pantalla_task(void *pvParameters);
void teclado_task(void *pvParameters);
int text_length(const char* text);
int16_t get_centered_position(const char* text);
void dibujar_menu_principal();
void dibujar_menu_configuracion();
void dibujar_menu_registrar_usuario();
void dibujar_ingresar_cedula();
void dibujar_confirmar_cedula();
void dibujar_ingresar_huella();
void dibujar_ingresar_pin();
void dibujar_seleccionar_tipo();
void dibujar_resumen_registro();

void app_main() {
    xTaskCreate(teclado_task, "TECLADO", 4096, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(pantalla_task, "PANTALLA", 4096, NULL, 5, NULL);
}

void teclado_task(void *pvParameters) {
    gpio_num_t keypad[8] = {6, 7, 15, 16, 3, 14, 13, 12};
    
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
                        estado_actual = ESTADO_CONFIGURACION;
                        opcion_seleccionada = 1;
                    }
                    break;
                case ESTADO_CONFIGURACION:
                    if(keypressed >= '1' && keypressed <= '3') {
                        opcion_seleccionada = keypressed - '0';
                    } else if(keypressed == 'A' && opcion_seleccionada == 1) {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                        memset(&usuario_actual, 0, sizeof(DatosUsuario));
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_PRINCIPAL;
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
                        usuario_actual.huella_registrada = true;
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
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
                case ESTADO_RESUMEN_REGISTRO:
                    if(keypressed == 'A') {
                        // Aquí iría la lógica para guardar el usuario
                        ESP_LOGI("REGISTRO", "Usuario registrado");
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
                        memset(&usuario_actual, 0, sizeof(DatosUsuario));
                    } else if(keypressed == 'B') {
                        estado_actual = ESTADO_REGISTRAR_USUARIO;
                        opcion_seleccionada = 1;
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


void dibujar_menu_principal() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("BIENVENIDO"), 0, "BIENVENIDO", ST7735_WHITE, ST7735_BLACK, 1);
    char fecha_hora[30];
    snprintf(fecha_hora, sizeof(fecha_hora), "Agosto 10 de 2024 20:00");
    TFTdrawText(get_centered_position(fecha_hora), SCREEN_HEIGHT/2, fecha_hora, ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "C: Configuracion", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_menu_configuracion() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("CONFIGURACION"), 0, "CONFIGURACION", ST7735_WHITE, ST7735_BLACK, 1);
    const char* opciones[] = {"1. REGISTRAR USUARIO", "2. BUSCAR USUARIO", "3. CONF. AVANZADA"};
    for(int i = 0; i < 3; i++) {
        uint16_t bgColor = (i == opcion_seleccionada - 1) ? ST7735_BLUE : ST7735_BLACK;
        uint16_t textColor = ST7735_WHITE;
        TFTdrawText(0, 30 + i*20, (char*)opciones[i], textColor, bgColor, 1);
    }
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Entrar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_menu_registrar_usuario() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("REGISTRAR USUARIO"), 0, "REGISTRAR USUARIO", ST7735_WHITE, ST7735_BLACK, 1);
    const char* opciones[] = {"1. CEDULA", "2. HUELLA", "3. PIN", "4. TIPO"};
    for(int i = 0; i < 4; i++) {
        uint16_t bgColor = (i == opcion_seleccionada - 1) ? ST7735_BLUE : ST7735_BLACK;
        uint16_t textColor = ST7735_WHITE;
        TFTdrawText(0, 20 + i*20, (char*)opciones[i], textColor, bgColor, 1);
        
        char valor[30] = "NULO";
        switch(i) {
            case 0: 
                if(strlen(usuario_actual.cedula) > 0) 
                    snprintf(valor, sizeof(valor), "%s", usuario_actual.cedula);
                break;
            case 1: 
                if(usuario_actual.huella_registrada)
                    snprintf(valor, sizeof(valor), "REGISTRADA");
                break;
            case 2:
                if(strlen(usuario_actual.pin) > 0)
                    snprintf(valor, sizeof(valor), "%s", usuario_actual.pin);
                break;
            case 3:
                if(strlen(usuario_actual.tipo) > 0)
                    snprintf(valor, sizeof(valor), "%s", usuario_actual.tipo);
                break;
        }
        TFTdrawText(SCREEN_WIDTH/2, 20 + i*20, valor, ST7735_WHITE, ST7735_BLACK, 1);
    }
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Entrar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_ingresar_cedula() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("INGRESAR CEDULA"), 0, "INGRESAR CEDULA", ST7735_WHITE, ST7735_BLACK, 1);
    
    char cedula_display[21] = {0};
    strncpy(cedula_display, usuario_actual.cedula, sizeof(cedula_display) - 1);
    for (int i = strlen(cedula_display); i < 20; i++) {
        cedula_display[i] = '_';
    }
    
    TFTdrawText(get_centered_position(cedula_display), SCREEN_HEIGHT / 2 - CHAR_HEIGHT / 2, cedula_display, ST7735_WHITE, ST7735_BLACK, 1);
    
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Confirmar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_ingresar_huella() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("INGRESAR HUELLA"), 0, "INGRESAR HUELLA", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("COLOQUE SU HUELLA"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "COLOQUE SU HUELLA", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("EN EL SENSOR"), SCREEN_HEIGHT / 2 + CHAR_HEIGHT, "EN EL SENSOR", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Confirmar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_ingresar_pin() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("INGRESAR PIN"), 0, "INGRESAR PIN", ST7735_WHITE, ST7735_BLACK, 1);
    
    char pin_display[5] = "____";
    for (int i = 0; i < strlen(usuario_actual.pin); i++) {
        pin_display[i] = '*';
    }
    
    TFTdrawText(get_centered_position(pin_display), SCREEN_HEIGHT / 2 - CHAR_HEIGHT / 2, pin_display, ST7735_WHITE, ST7735_BLACK, 1);
    
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Confirmar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_seleccionar_tipo() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("SELECCIONAR TIPO"), 0, "SELECCIONAR TIPO", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "1. ADMINISTRADOR", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT / 2 + CHAR_HEIGHT, "2. USUARIO", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_resumen_registro() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("RESUMEN REGISTRO"), 0, "RESUMEN REGISTRO", ST7735_WHITE, ST7735_BLACK, 1);
    
    char resumen[4][30];
    snprintf(resumen[0], sizeof(resumen[0]), "Cedula: %s", strlen(usuario_actual.cedula) > 0 ? usuario_actual.cedula : "NULO");
    snprintf(resumen[1], sizeof(resumen[1]), "Huella: %s", usuario_actual.huella_registrada ? "REGISTRADA" : "NULO");
    snprintf(resumen[2], sizeof(resumen[2]), "PIN: %s", strlen(usuario_actual.pin) > 0 ? "****" : "NULO");
    snprintf(resumen[3], sizeof(resumen[3]), "Tipo: %s", strlen(usuario_actual.tipo) > 0 ? usuario_actual.tipo : "NULO");
    
    for (int i = 0; i < 4; i++) {
        TFTdrawText(0, 30 + i*20, resumen[i], ST7735_WHITE, ST7735_BLACK, 1);
    }
    
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Guardar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
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
