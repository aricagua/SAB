
#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

// In draw.h
extern EstadoMenu estado_actual;
extern int opcion_seleccionada;
extern DatosUsuario usuario_actual;
extern char admin_pin_input[5];
extern uint16_t page_to_delete;
extern bool as608_initialized;
extern int input_index; 

void dibujar_esperando_huella(void);
void dibujar_asistencia_registrada(uint16_t page_number);
void dibujar_error_registro_asistencia(void);
void dibujar_huella_no_reconocida(void);
void dibujar_error_verificacion(void);
void dibujar_menu_principal();
void dibujar_menu_configuracion();
void dibujar_ingresar_pin_admin();
void dibujar_menu_registrar_usuario();
void dibujar_ingresar_cedula();
void dibujar_ingresar_huella();
void dibujar_ingresar_pin();
void dibujar_seleccionar_tipo();
void dibujar_asistencia();
void dibujar_resumen_registro();
void dibujar_error_registro();
void dibujar_registro_exitoso();
void dibujar_borrar_huella();
void dibujar_borrado_exitoso();
void dibujar_error_borrado();
void dibujar_resetear_sistema(void);
void dibujar_reset_exitoso(void);
void dibujar_error_reset(void);
int text_length(const char* text);
int16_t get_centered_position(const char* text);

#endif // MENU_H

