#include "nvs_user_data_module.h"
//#include "config.h" // For NVS_NAMESPACE
#include "esp_log.h"
#include "sntp_time_module.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "common.h" // For DatosUsuario struct
#include <inttypes.h> // For PRIu32
#include <stdio.h> // For snprintf
//#include "sntp_time_module.h" // For get_current_time
#include <time.h> // For struct tm

#define NVS_NAMESPACE "user_data"

static const char *TAG = "nvs_user_data";

esp_err_t erase_all_user_data_nvs() {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS for user data
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for user data: %s", esp_err_to_name(err));
        return err;
    }

    // Erase all user data keys
    err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error erasing all keys in user data namespace: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error committing NVS changes for user data: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "All user data erased successfully");
        }
    }

    // Close NVS for user data
    nvs_close(nvs_handle);

    // Open NVS for attendance log (assuming ATTENDANCE_LOG_NAMESPACE exists)
    err = nvs_open("attendance_log", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for attendance log: %s", esp_err_to_name(err));
        return err;
    }

    // Erase all attendance log keys
    err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error erasing all keys in attendance log namespace: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error committing NVS changes for attendance log: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "All attendance log erased successfully");
        }
    }

    // Close NVS for attendance log
    nvs_close(nvs_handle);

    return err;
}


esp_err_t register_attendance_nvs(const DatosUsuario *usuario) {
    struct tm timeinfo;
    get_current_time(&timeinfo);

    // Create a log entry
    char log_entry[128];
    snprintf(log_entry, sizeof(log_entry), "%04d-%02d-%02d %02d:%02d:%02d, %s, %s",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
             usuario->cedula, usuario->tipo);

    // Open NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("attendance_log", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Get the current log count
    uint32_t log_count = 0;
    err = nvs_get_u32(nvs_handle, "log_count", &log_count);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error reading log count: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Create a key for this log entry
    char key[16];
    snprintf(key, sizeof(key), "log_%" PRIu32, log_count);

    // Save the log entry
    err = nvs_set_str(nvs_handle, key, log_entry);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving log entry: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Increment and save the log count
    log_count++;
    err = nvs_set_u32(nvs_handle, "log_count", log_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving log count: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    }

    // Close NVS
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Attendance registered for user: %s", usuario->cedula);
    }

    return err;
}

esp_err_t load_user_data_nvs(DatosUsuario *usuario, uint16_t huella_pagina) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Create the key for the user
    char key[16];
    snprintf(key, sizeof(key), "user_%d", huella_pagina);

    // Read the user data from NVS
    size_t required_size = sizeof(DatosUsuario);
    err = nvs_get_blob(nvs_handle, key, usuario, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading user data from NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "User data loaded successfully");
    }

    // Close NVS
    nvs_close(nvs_handle);

    return err;
}

esp_err_t save_user_data_nvs(const DatosUsuario *usuario) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Create a key for the user based on their fingerprint page number
    char key[16];
    snprintf(key, sizeof(key), "user_%d", usuario->huella_pagina);

    // Write the user data to NVS
    err = nvs_set_blob(nvs_handle, key, usuario, sizeof(DatosUsuario));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing user data to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit the changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "User data saved successfully");
    }

    // Close NVS
    nvs_close(nvs_handle);

    return err;
}
