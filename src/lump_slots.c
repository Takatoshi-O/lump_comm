#include "lump_slots.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    uint8_t seq;
    uint8_t mode;
    uint8_t sensorID;
    int16_t values[4];
    bool dirty;                 /* まだ一度も送信されていない新しい値がある */
    SemaphoreHandle_t mutex;
} lump_slot_t;

static lump_slot_t s_slots[LUMP_TYPE_MAX];
static uint8_t s_rr_cursor = 0;

void lump_slots_init(void) {
    for (int i = 0; i < LUMP_TYPE_MAX; i++) {
        memset(&s_slots[i], 0, sizeof(lump_slot_t));
        s_slots[i].mutex = xSemaphoreCreateMutex();
    }
    s_rr_cursor = 0;
}

void lump_slots_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                        int16_t v1, int16_t v2, int16_t v3, int16_t v4) {
    if (type >= LUMP_TYPE_MAX) return;

    lump_slot_t *slot = &s_slots[type];
    if (xSemaphoreTake(slot->mutex, portMAX_DELAY) == pdTRUE) {
        slot->seq++;
        slot->mode = mode & 0x1F; /* 下位5bitのみ使う */
        slot->sensorID = sensorID;
        slot->values[0] = v1;
        slot->values[1] = v2;
        slot->values[2] = v3;
        slot->values[3] = v4;
        slot->dirty = true;
        xSemaphoreGive(slot->mutex);
    }
}

/* slotの内容をパケット形式 (byte0=種別+モード, byte1=センサーID, byte2=seq, byte3-10=int16x4) にパックする */
static void pack_slot(lump_sensor_type_t type, const lump_slot_t *slot, uint8_t out[LUMP_PAYLOAD_LEN]) {
    out[0] = (uint8_t)((type & 0x07) << 5) | (slot->mode & 0x1F);
    out[1] = slot->sensorID;
    out[2] = slot->seq;
    for (int i = 0; i < 4; i++) {
        int16_t v = slot->values[i];
        out[3 + i * 2]     = (uint8_t)(v & 0xFF);        /* リトルエンディアン */
        out[3 + i * 2 + 1] = (uint8_t)((v >> 8) & 0xFF);
    }
}

bool lump_slots_pick_next(uint8_t out[LUMP_PAYLOAD_LEN]) {
    for (int i = 0; i < LUMP_TYPE_MAX; i++) {
        uint8_t idx = (s_rr_cursor + i) % LUMP_TYPE_MAX;
        lump_slot_t *slot = &s_slots[idx];

        if (!slot->dirty) continue;

        if (xSemaphoreTake(slot->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            pack_slot((lump_sensor_type_t)idx, slot, out);
            slot->dirty = false;
            xSemaphoreGive(slot->mutex);
            s_rr_cursor = (idx + 1) % LUMP_TYPE_MAX;
            return true;
        }
    }
    return false;
}
