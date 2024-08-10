#include "sd_mmc.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

static const char *TAG = "sd_mmc";

esp_err_t configurar_sd(void)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 5000;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SD card mounted successfully");
    return ESP_OK;
}

esp_err_t leer_mapa_bits(const char* filename, bitmap_t* bitmap)
{
    char filepath[64];
    snprintf(filepath, sizeof(filepath), MOUNT_POINT "/%s", filename);

    FILE* f = fopen(filepath, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filepath);
        return ESP_FAIL;
    }

    // Leer las dimensiones del mapa de bits (asumiendo un formato simple)
    fread(&bitmap->width, sizeof(int), 1, f);
    fread(&bitmap->height, sizeof(int), 1, f);

    // Calcular el tamaño de los datos y asignar memoria
    int data_size = bitmap->width * bitmap->height;
    bitmap->data = malloc(data_size);
    if (bitmap->data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for bitmap data");
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    // Leer los datos del mapa de bits
    size_t read_bytes = fread(bitmap->data, 1, data_size, f);
    fclose(f);

    if (read_bytes != data_size) {
        ESP_LOGE(TAG, "Failed to read complete bitmap data");
        free(bitmap->data);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Bitmap read successfully: %dx%d", bitmap->width, bitmap->height);
    return ESP_OK;
}

void liberar_mapa_bits(bitmap_t* bitmap)
{
    if (bitmap->data != NULL) {
        free(bitmap->data);
        bitmap->data = NULL;
    }
    bitmap->width = 0;
    bitmap->height = 0;
}

esp_err_t desmontar_sd(void)
{
    esp_err_t ret = esp_vfs_fat_sdmmc_unmount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SD card unmounted successfully");
    return ESP_OK;
}

