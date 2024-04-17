/**
 * Simple BLE Beacon
 *
 * This is a very simple "beacon" that doesn't follow any of the existing
 * beacon standards. I wrote it as a part of learning how BLE advertisements
 * worked using NimBLE.
 *
 * NOTE: Since this is just a beacon sending one specific advertisement, we
 * are not going to use GAP and GATT server.
 *
 * SPDX-FileCopyrightText: 2024 Jewel Mahanta <jewelmahanta@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 */
#include "esp_log.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// NimBLE
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"

uint8_t ble_own_addr_type;
static const char *DEVICE_NAME = "BLE Beacon";

// LOG TAG
static const char *TAG = "BLE_Beacon";

static void ble_advertise(void) {
    int rc;
    struct ble_hs_adv_fields fields;
    // The first 2 bytes of Manufacturer Specific data is interpreted by the
    // scanner as part of Manufacturer Code. This function left pads a string
    // with a set prefix of FF. NOTE: custom_data has a max length of 14
    // characters to fit in 31 bytes ADV PDU
    char *custom_data = "FFBeacon 1 1";
    printf("custom: %s\n", custom_data);

    // Set the advertizing fields
    memset(&fields, 0, sizeof(fields));

    // BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE (0x06)
    // 1 byte length + 1 byte type + 1 byte payload = 3 bytes
    fields.flags = 0x06;

    // 1 byte length + 1 byte type + 10 bytes data = 12 bytes
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;

    // 1 byte length + 1 byte type + 1 - 14 bytes data = 3 - 16 bytes
    fields.mfg_data = (uint8_t *)custom_data;
    fields.mfg_data_len = strlen(custom_data);

    // 26 bytes total payload size
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertizing data: %d\n", rc);
        return;
    }

    // Begin advertizing
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    //  We want -> ADV_NONCONN_IND
    // LIMITED and GENERAL discoverable talks about the time and not
    // discoverability characteristics. LIMITED means after 30s we are
    // not discoverable anymore until reboot/reset.
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // Discoverable
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON; // NON Connectable
    rc = ble_gap_adv_start(ble_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                           NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting advertizing: %d\n", rc);
        return;
    }
}

static void ble_on_reset(int reason) {
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

/**
 * NOTE: We can only use the Nimble stack after the host and the controller sync
 * up.
 */
static void ble_on_sync(void) {
    int rc;
    // First we get our address and then we advertize.
    rc = ble_hs_util_ensure_addr(0);
    // abort program if we can't get an address
    assert(rc == 0);
    rc = ble_hs_id_infer_auto(0, &ble_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error inferring error type: %d\n", rc);
        return;
    }
    ble_advertise();
}

void ble_host_task(void) {
    ESP_LOGI(TAG, "BLE host task started");
    // This function will return only when nimble_port_stop() is executed
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    // We will initialize NVS Flash (non-volatile storage) first.
    // Nimble uses NVS to store our physical calibration data.
    int rc;
    ESP_LOGI(TAG, "::Initializing BLE");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // Initialize the host and controller stack
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "::Failed to initialize Nimble stack \n");
        return;
    }
    // Initialize the Nimble host configuration
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    nimble_port_freertos_init(ble_host_task);
    while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "hearbeat...");
    }
}
