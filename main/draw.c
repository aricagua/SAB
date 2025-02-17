/*draw.c*/
#include "draw.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <tft.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "sntp_time_module.h"
#include "nvs_user_data_module.h"
#include <nvs.h>        // Add this line
#include <nvs_flash.h>  // Add this line (if not already initialized elsewhere)
#include <esp_log.h>    // Add this line


static const char *TAG = "draw";

int opcion_seleccionada = 1;

void dibujar_menu_principal() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("BIENVENIDO"), 0, "BIENVENIDO", ST7735_WHITE, ST7735_BLACK, 1);

    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Format date and time
    char fecha_hora[30];
    strftime(fecha_hora, sizeof(fecha_hora), "%b %d %Y %H:%M", &timeinfo);

    TFTdrawText(get_centered_position(fecha_hora), SCREEN_HEIGHT/2 - CHAR_HEIGHT, fecha_hora, ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("*: Asistencia"), SCREEN_HEIGHT/2 + CHAR_HEIGHT, "*: Asistencia", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "C: Configuracion", ST7735_WHITE, ST7735_BLACK, 1);
}



void dibujar_ingresar_pin_admin() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("INGRESE PIN ADMIN"), 0, "INGRESE PIN ADMIN", ST7735_WHITE, ST7735_BLACK, 1);
    
    char pin_display[5] = "____";
    for (int i = 0; i < strlen(admin_pin_input); i++) {
        pin_display[i] = '*';
    }
    
    TFTdrawText(get_centered_position(pin_display), SCREEN_HEIGHT / 2 - CHAR_HEIGHT / 2, pin_display, ST7735_WHITE, ST7735_BLACK, 1);
    
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Confirmar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
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
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT * 2, "A: Entrar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "C: Guardar", ST7735_WHITE, ST7735_BLACK, 1);
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
    if (as608_initialized) {
        TFTdrawText(get_centered_position("COLOQUE SU HUELLA"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "COLOQUE SU HUELLA", ST7735_WHITE, ST7735_BLACK, 1);
        TFTdrawText(get_centered_position("EN EL SENSOR"), SCREEN_HEIGHT / 2 + CHAR_HEIGHT, "EN EL SENSOR", ST7735_WHITE, ST7735_BLACK, 1);
    } else {
        TFTdrawText(get_centered_position("SENSOR NO"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "SENSOR NO", ST7735_RED, ST7735_BLACK, 1);
        TFTdrawText(get_centered_position("INICIALIZADO"), SCREEN_HEIGHT / 2 + CHAR_HEIGHT, "INICIALIZADO", ST7735_RED, ST7735_BLACK, 1);
    }
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

void dibujar_error_registro() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("ERROR"), 0, "ERROR", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("FALTAN DATOS"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "FALTAN DATOS", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("O HUBO UN ERROR"), SCREEN_HEIGHT / 2 + CHAR_HEIGHT, "O HUBO UN ERROR", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_registro_exitoso() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("REGISTRO"), 0, "REGISTRO", ST7735_GREEN, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("EXITOSO"), SCREEN_HEIGHT / 2, "EXITOSO", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_borrar_huella() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("BORRAR HUELLA"), 0, "BORRAR HUELLA", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("Ingrese pagina"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT * 2, "Ingrese pagina", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("a borrar:"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "a borrar:", ST7735_WHITE, ST7735_BLACK, 1);
    
    char page_str[7];  // Increased buffer size to accommodate up to 5 digits + null terminator
    snprintf(page_str, sizeof(page_str), "%u", page_to_delete);  // Use %u for unsigned int
    TFTdrawText(get_centered_position(page_str), SCREEN_HEIGHT / 2 + CHAR_HEIGHT, page_str, ST7735_WHITE, ST7735_BLACK, 1);
    
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT * 2, "A: Borrar B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "D: Corregir", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_borrado_exitoso() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("HUELLA BORRADA"), 0, "HUELLA BORRADA", ST7735_GREEN, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("EXITOSAMENTE"), SCREEN_HEIGHT / 2, "EXITOSAMENTE", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_error_borrado() {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("ERROR"), 0, "ERROR", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("AL BORRAR"), SCREEN_HEIGHT / 2 - CHAR_HEIGHT, "AL BORRAR", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("HUELLA"), SCREEN_HEIGHT / 2 + CHAR_HEIGHT, "HUELLA", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "A: Volver", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_resetear_sistema(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 20, "RESETEAR SISTEMA", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(10, 40, "A: Confirmar", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(10, 60, "B: Cancelar", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(10, 80, "ADVERTENCIA:", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 100, "Se borraran todos", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 120, "los datos", ST7735_RED, ST7735_BLACK, 1);
}

void dibujar_reset_exitoso(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 60, "SISTEMA RESETEADO", ST7735_GREEN, ST7735_BLACK, 1);
    TFTdrawText(10, 80, "EXITOSAMENTE", ST7735_GREEN, ST7735_BLACK, 1);
    TFTdrawText(10, 100, "A: Continuar", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_error_reset(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 60, "ERROR AL RESETEAR", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 80, "EL SISTEMA", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 100, "A: Continuar", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_menu_configuracion(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("CONFIGURACION"), 0, "CONFIGURACION", ST7735_WHITE, ST7735_BLACK, 1);
    const char* opciones[] = {
        "1. Registrar Usuario",
        "2. Buscar Usuario",
        "3. Config. Avanzada",
        "4. Resetear Sistema",
        "5. Ver registro"
    };
    for(int i = 0; i < 5; i++) {
        uint16_t bgColor = (i == opcion_seleccionada - 1) ? ST7735_BLUE : ST7735_BLACK;
        uint16_t textColor = ST7735_WHITE;
        TFTdrawText(0, 20 + i*12, (char*)opciones[i], textColor, bgColor, 1);
    }
    
    // Bottom instructions with smaller font
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT * 3, "A: Seleccionar", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT * 2, "B: Volver", ST7735_WHITE, ST7735_BLACK, 1);
    
    // Selected option display
    char opcion_str[2];
    sprintf(opcion_str, "%d", opcion_seleccionada);
    TFTdrawText(0, SCREEN_HEIGHT - CHAR_HEIGHT, "Opcion: ", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(CHAR_WIDTH * 8, SCREEN_HEIGHT - CHAR_HEIGHT, opcion_str, ST7735_GREEN, ST7735_BLACK, 1);
}

void dibujar_esperando_huella(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 40, "COLOQUE SU DEDO", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(10, 60, "y presione A:", ST7735_CYAN, ST7735_BLACK, 1);
    TFTdrawText(10, 80, "para escanear asistencia", ST7735_CYAN, ST7735_BLACK, 1);
    TFTdrawText(10, 120, "B: Cancelar", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_asistencia_registrada(uint16_t page_number)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 40, "ASISTENCIA", ST7735_GREEN, ST7735_BLACK, 1);
    TFTdrawText(10, 60, "REGISTRADA", ST7735_GREEN, ST7735_BLACK, 1);
    
    char user_info[30];
    snprintf(user_info, sizeof(user_info), "Usuario ID: %d", page_number);
    TFTdrawText(10, 90, user_info, ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_error_registro_asistencia(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 40, "ERROR AL REGISTRAR", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 60, "LA ASISTENCIA", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 100, "Intente nuevamente", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_huella_no_reconocida(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 40, "HUELLA NO", ST7735_YELLOW, ST7735_BLACK, 1);
    TFTdrawText(10, 60, "RECONOCIDA", ST7735_YELLOW, ST7735_BLACK, 1);
    TFTdrawText(10, 100, "Intente nuevamente", ST7735_WHITE, ST7735_BLACK, 1);
}

void dibujar_error_verificacion(void)
{
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(10, 40, "ERROR DE", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 60, "VERIFICACION", ST7735_RED, ST7735_BLACK, 1);
    TFTdrawText(10, 100, "Intente nuevamente", ST7735_WHITE, ST7735_BLACK, 1);
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

void dibujar_ver_registro(uint32_t start_index) {
    TFTfillScreen(ST7735_BLACK);
    TFTdrawText(get_centered_position("REGISTRO"), 0, "REGISTRO", ST7735_WHITE, ST7735_BLACK, 1);
    TFTdrawText(get_centered_position("ASISTENCIA"), 10, "ASISTENCIA", ST7735_WHITE, ST7735_BLACK, 1);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("attendance_log", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        TFTdrawText(0, SCREEN_HEIGHT/2, "Error NVS", ST7735_RED, ST7735_BLACK, SMALL_FONT_SCALE);
        return;
    }

    uint32_t log_count = 0;
    nvs_get_u32(nvs_handle, "log_count", &log_count);

    for (int i = 0; i < LOGS_PER_PAGE; i++) {
        uint32_t index = start_index + i;
        if (index >= log_count) break;

        char key[16];
        snprintf(key, sizeof(key), "log_%" PRIu32, index);

        char log_entry[128];
        size_t required_size = sizeof(log_entry);
        err = nvs_get_str(nvs_handle, key, log_entry, &required_size);
        if (err == ESP_OK) {
            int year, month, day, hour, min;
            char cedula[20], tipo[20];
            sscanf(log_entry, "%d-%d-%d %d:%d:%*d, %19[^,], %19s", 
                   &year, &month, &day, &hour, &min, cedula, tipo);
            
            char date_line[MAX_DISPLAY_LINE];
            char info_line[MAX_DISPLAY_LINE];
            snprintf(date_line, sizeof(date_line), "%02d/%02d/%02d %02d:%02d", 
                     day, month, year % 100, hour, min);
            snprintf(info_line, sizeof(info_line), "%.10s %.10s", cedula, tipo);
            
            TFTdrawText(0, 25 + i*20, date_line, ST7735_YELLOW, ST7735_BLACK, SMALL_FONT_SCALE);
            TFTdrawText(0, 25 + i*20 + 10, info_line, ST7735_CYAN, ST7735_BLACK, SMALL_FONT_SCALE);
        }
    }

    nvs_close(nvs_handle);

    char nav_text[64];
    snprintf(nav_text, sizeof(nav_text), "B:Atrs A:Sig C:Menu %lu/%lu", 
             (unsigned long)(start_index/LOGS_PER_PAGE + 1), 
             (unsigned long)((log_count + LOGS_PER_PAGE - 1) / LOGS_PER_PAGE));
    TFTdrawText(0, SCREEN_HEIGHT - 10, nav_text, ST7735_WHITE, ST7735_BLACK, SMALL_FONT_SCALE);

    ESP_LOGI(TAG, "VER_REGISTRO: Start index: %lu, Log count: %lu", (unsigned long)start_index, (unsigned long)log_count);
}





