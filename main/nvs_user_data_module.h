#ifndef NVS_USER_DATA_MODULE_H
#define NVS_USER_DATA_MODULE_H

#include "esp_err.h"
#include "common.h" // For DatosUsuario struct
#include <stdint.h> // For uint16_t

esp_err_t erase_all_user_data_nvs(void);
esp_err_t load_user_data_nvs(DatosUsuario *usuario, uint16_t huella_pagina);
esp_err_t register_attendance_nvs(const DatosUsuario *usuario);
esp_err_t save_user_data_nvs(const DatosUsuario *usuario);

#endif // NVS_USER_DATA_MODULE_H