#include "lump_comm.h"
#include "lump_protocol.h"
#include "lump_message.h"
#include "lump_bitbang.h"
#include "lump_slots.h"

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#define LUMP_DEBUG 0
#define LUMP_UART_NUM UART_NUM_1

static const char *TAG = "lump_device";

static uint8_t g_rx_command[LUMP_PAYLOAD_LEN] = {0};
static SemaphoreHandle_t g_rx_mutex;

/* 何も新しいセンサーデータがない時に送信する
 * (ハートビートとして使う) */
static const uint8_t ack[LUMP_PAYLOAD_LEN] = {0};

static volatile bool s_connected = false;

/* ===================== 起動シーケンス ===================== */

static void log_and_send(const char *label, const uint8_t *buf, int n) {
#if LUMP_DEBUG
    char hex[3 * 40 + 1] = {0};
    int pos = 0;
    for (int i = 0; i < n && pos < (int)sizeof(hex) - 3; i++) {
        pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", buf[i]);
    }
    ESP_LOGI(TAG, "TX %s: %s", label, hex);
#endif
    lump_bb_send_bytes(buf, n);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void send_handshake_sequence(void) {
    uint8_t buf[LUMP_MSGBUF_MAX];
    int n;

    lump_bb_send_byte(LUMP_BYTE_SYNC);
    //ESP_LOGI(TAG, "TX SYNC: 00");
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t type_id = LUMP_DEVICE_TYPE_ID;
    n = lump_msg_build_cmd_or_data(buf, LUMP_MSG_CMD, LUMP_CMD_TYPE, &type_id, 1);
    log_and_send("CMD_TYPE", buf, n);

    /* モードは1つのみ(0番)。パケット全体をこのモード内で多重化しているため、
     * LUMP自体のモード数は増やさない(モード切替のオーバーヘッドを避ける) */
    uint8_t modes[2] = {0x00, 0x00};
    n = lump_msg_build_cmd_or_data(buf, LUMP_MSG_CMD, LUMP_CMD_MODES, modes, 2);
    log_and_send("CMD_MODES", buf, n);

    uint8_t speed[4] = {0x00, 0xC2, 0x01, 0x00}; /* 115200 (リトルエンディアン) */
    n = lump_msg_build_cmd_or_data(buf, LUMP_MSG_CMD, LUMP_CMD_SPEED, speed, 4);
    log_and_send("CMD_SPEED", buf, n);

    uint8_t version[8] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01};
    n = lump_msg_build_cmd_or_data(buf, LUMP_MSG_CMD, LUMP_CMD_VERSION, version, 8);
    log_and_send("CMD_VERSION", buf, n);

    n = lump_msg_build_info(buf, LUMP_MODE_0, LUMP_INFO_NAME,
                             (const uint8_t *)LUMP_MODE_NAME, strlen(LUMP_MODE_NAME));
    log_and_send("INFO_NAME", buf, n);

    uint8_t mapping[2] = {0x10, 0x10}; /* 読み書き両対応 */
    n = lump_msg_build_info(buf, LUMP_MODE_0, LUMP_INFO_MAPPING, mapping, 2);
    log_and_send("INFO_MAPPING", buf, n);

    uint8_t format[4] = {LUMP_PAYLOAD_LEN, LUMP_DATA8, 3, 0};
    n = lump_msg_build_info(buf, LUMP_MODE_0, LUMP_INFO_FORMAT, format, 4);
    log_and_send("INFO_FORMAT", buf, n);

    vTaskDelay(pdMS_TO_TICKS(10));

    /* デバイス自身から紹介シーケンス終了のACKを送る(実機検証で必須と判明) */
    ESP_LOGI(TAG, "TX (own) ACK: 04");
    lump_bb_send_byte(LUMP_BYTE_ACK);
}

/* ===================== 同期フェーズ(ビットバンギング) ===================== */

static bool wait_for_connection(void) {
    lump_bb_init(LUMP_GPIO_TX, LUMP_GPIO_RX);

    ESP_LOGI(TAG, "waiting for host detect pulses...");

    unsigned int pulsecnt = 0;
    while (1) {
        unsigned int dat = lump_bb_recv_byte();
        if (dat == LUMP_BYTE_DETECT) {
            pulsecnt++;
            if (pulsecnt >= 5) break;
        } else if (dat != 0xFFFF) {
            pulsecnt = 0;
        }
        vTaskDelay(1);
    }

    ESP_LOGI(TAG, "host detected. responding...");

    esp_rom_delay_us(LUMP_SYNC_BAUD_US * 10 * 3);
    lump_bb_send_byte(LUMP_BYTE_DETECT);

    lump_bb_set_tx_level(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    lump_bb_set_tx_level(1);

    vTaskDelay(pdMS_TO_TICKS(10));

    send_handshake_sequence();
    
    TickType_t ack_wait_start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - ack_wait_start) < pdMS_TO_TICKS(1000)) {
        unsigned int dat = lump_bb_recv_byte();
        if (dat == 0xFFFF) continue;

        ESP_LOGI(TAG, "(waiting for ACK) got: 0x%02X", dat);
        if (dat == LUMP_BYTE_ACK) {
            ESP_LOGI(TAG, "ACK received!");
            return true;
        }
    }
    ESP_LOGW(TAG, "timed out waiting for ACK");
    return false;
}

/* ===================== DATAフェーズ(ハードウェアUART, 115200bps) ===================== */

static void init_hw_uart(void) {
    const uart_config_t cfg = {
        .baud_rate  = LUMP_RUN_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(LUMP_UART_NUM, 256, 256, 0, NULL, 0);
    uart_param_config(LUMP_UART_NUM, &cfg);
    uart_set_pin(LUMP_UART_NUM, LUMP_GPIO_TX, LUMP_GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static bool hw_receive_message(uint8_t *out_header, uint8_t *out_payload, uint8_t *out_len,
                                TickType_t timeout) {
    uint8_t header;
    if (uart_read_bytes(LUMP_UART_NUM, &header, 1, timeout) != 1) return false;

    if (header == LUMP_BYTE_NACK || header == LUMP_BYTE_ACK || header == LUMP_BYTE_SYNC) {
        *out_header = header;
        *out_len = 0;
        return true;
    }

    uint8_t len = lump_msg_payload_len_from_header(header);
    uint8_t payload[32];
    if (uart_read_bytes(LUMP_UART_NUM, payload, len, pdMS_TO_TICKS(50)) != len) return false;

    uint8_t checksum;
    if (uart_read_bytes(LUMP_UART_NUM, &checksum, 1, pdMS_TO_TICKS(50)) != 1) return false;

    if (lump_msg_checksum(header, payload, len) != checksum) {
#if LUMP_DEBUG
        ESP_LOGW(TAG, "checksum mismatch");
#endif
        return false;
    }

    *out_header = header;
    memcpy(out_payload, payload, len);
    *out_len = len;
    return true;
}

static void data_phase(void) {
    init_hw_uart();
    ESP_LOGI(TAG, "entering data phase (115200 baud, hardware UART)");

    TickType_t last_rx_seen = xTaskGetTickCount();
    TickType_t last_tx = xTaskGetTickCount();
    uint8_t pending_ext_mode = 0;
    bool have_pending_ext_mode = false;

    while (1) {
        uint8_t header, payload[32], len;
        bool got = hw_receive_message(&header, payload, &len, pdMS_TO_TICKS(5));
        bool nack_now = false;

        if (got) {
            last_rx_seen = xTaskGetTickCount();
            if (header == LUMP_BYTE_NACK) {
                nack_now = true;
            } else if (len > 0) {
                uint8_t msg_type    = header & LUMP_MSG_TYPE_MASK;
                uint8_t cmd_or_mode = header & LUMP_MSG_LOWER_MASK;
                if (msg_type == LUMP_MSG_CMD && cmd_or_mode == LUMP_CMD_EXT_MODE) {
                    pending_ext_mode = payload[0];
                    have_pending_ext_mode = true;
                } else if (msg_type == LUMP_MSG_DATA) {
                    uint8_t actual_mode = cmd_or_mode + (have_pending_ext_mode ? pending_ext_mode : 0);
                    have_pending_ext_mode = false;
                    if (actual_mode == 0 && len >= LUMP_PAYLOAD_LEN) {
                        if (xSemaphoreTake(g_rx_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                            memcpy(g_rx_command, payload, LUMP_PAYLOAD_LEN);
                            xSemaphoreGive(g_rx_mutex);
                        }
                    }
                }
            }
        }

        if ((xTaskGetTickCount() - last_rx_seen) > pdMS_TO_TICKS(300)) {
            ESP_LOGW(TAG, "host timeout, reconnecting");
            uart_driver_delete(LUMP_UART_NUM);
            return;
        }

        bool interval_elapsed = (xTaskGetTickCount() - last_tx) > pdMS_TO_TICKS(5);
        if (nack_now || interval_elapsed) {
            uint8_t payload_buf[LUMP_PAYLOAD_LEN];
            if (!lump_slots_pick_next(payload_buf)) {
                /* 新しいデータがなければ、ackをハートビートとして再送する */
                memcpy(payload_buf, ack, LUMP_PAYLOAD_LEN);
            }

            uint8_t buf[LUMP_MSGBUF_MAX];
            int n = lump_msg_build_cmd_or_data(buf, LUMP_MSG_DATA, LUMP_MODE_0, payload_buf, LUMP_PAYLOAD_LEN);
            uart_write_bytes(LUMP_UART_NUM, (const char *)buf, n);
            last_tx = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); /* タスクウォッチドッグ対策(IDLEタスクに実行機会を渡す) */
    }
}

/* ===================== タスク本体 ===================== */

static void lump_device_task(void *arg) {
    while (1) {
        if (wait_for_connection()) {
            s_connected = true;
            data_phase();
        }
        s_connected = false;
    }
}

/* ===================== 公開API ===================== */

void lump_device_start(void) {
    g_rx_mutex = xSemaphoreCreateMutex();
    lump_slots_init();
    if (CONFIG_FREERTOS_NUMBER_OF_CORES > 1)
    {
        xTaskCreatePinnedToCore(lump_device_task, "lump_device", 4096, NULL, 10, NULL, 1);
    }
    else
    {
        xTaskCreate(lump_device_task, "lump_device", 4096, NULL, 10, NULL);
    }
}

bool lump_device_is_connected(void) {
    return s_connected;
}

void lump_device_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                         int16_t v1, int16_t v2, int16_t v3, int16_t v4) {
    lump_slots_report(type, mode, sensorID, v1, v2, v3, v4);
}

void lump_device_get_command(uint8_t out[LUMP_PAYLOAD_LEN]) {
    if (xSemaphoreTake(g_rx_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(out, g_rx_command, LUMP_PAYLOAD_LEN);
        xSemaphoreGive(g_rx_mutex);
    }
}