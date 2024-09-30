// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

typedef enum {
    ESTADO_BIENVENIDA,
    ESTADO_PRINCIPAL,
    ESTADO_INGRESAR_PIN_ADMIN,
    ESTADO_CONFIGURACION,
    ESTADO_REGISTRAR_USUARIO,
    ESTADO_INGRESAR_CEDULA,
    ESTADO_INGRESAR_HUELLA,
    ESTADO_INGRESAR_PIN,
    ESTADO_SELECCIONAR_TIPO,
    ESTADO_RESUMEN_REGISTRO,
    ESTADO_ASISTENCIA,
    ESTADO_ERROR_REGISTRO,
    ESTADO_REGISTRO_EXITOSO,
    ESTADO_RESETEAR_SISTEMA,
    ESTADO_RESET_EXITOSO,
    ESTADO_ERROR_RESET,
    ESTADO_VER_REGISTRO
} EstadoMenu;

typedef struct {
    char cedula[20];
    bool huella_registrada;
    uint16_t huella_pagina;
    char pin[5];
    char tipo[20];
} DatosUsuario;

#endif // COMMON_H
