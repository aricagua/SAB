#ifndef SD_MMC_H
#define SD_MMC_H

#include <stdint.h>
#include "esp_err.h"

#define MOUNT_POINT "/sdcard"

// Estructura para almacenar datos de mapa de bits
typedef struct {
    uint8_t* data;
    int width;
    int height;
} bitmap_t;

/**
 * @brief Configura y monta la tarjeta SD
 *
 * Esta función inicializa el hardware SDMMC, monta el sistema de archivos FAT
 * y prepara la tarjeta SD para su uso.
 *
 * @return esp_err_t ESP_OK si la operación fue exitosa, código de error en caso contrario
 */
esp_err_t configurar_sd(void);

/**
 * @brief Lee un archivo de mapa de bits desde la tarjeta SD
 *
 * Esta función lee un archivo de mapa de bits desde la tarjeta SD y lo almacena
 * en una estructura bitmap_t para su uso posterior.
 *
 * @param filename Nombre del archivo a leer
 * @param bitmap Puntero a la estructura bitmap_t donde se almacenarán los datos
 * @return esp_err_t ESP_OK si la operación fue exitosa, código de error en caso contrario
 */
esp_err_t leer_mapa_bits(const char* filename, bitmap_t* bitmap);

/**
 * @brief Libera la memoria asignada para un mapa de bits
 *
 * Esta función libera la memoria asignada para los datos de un mapa de bits.
 *
 * @param bitmap Puntero a la estructura bitmap_t a liberar
 */
void liberar_mapa_bits(bitmap_t* bitmap);

/**
 * @brief Desmonta el sistema de archivos de la tarjeta SD
 *
 * Esta función desmonta de forma segura el sistema de archivos de la tarjeta SD.
 *
 * @return esp_err_t ESP_OK si la operación fue exitosa, código de error en caso contrario
 */
esp_err_t desmontar_sd(void);

#endif // SD_MMC_H
