#include "lump_command.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static lump_command_entry_t s_queue[LUMP_COMMAND_QUEUE_CAPACITY];
static size_t s_head = 0;   /* 次に書き込む位置(最新) */
static size_t s_tail = 0;   /* 次に取り出す位置(最古) */
static size_t s_count = 0;  /* 現在の件数 */
static SemaphoreHandle_t s_mutex;

void lump_command_init(void) {
    s_head = 0;
    s_tail = 0;
    s_count = 0;
    s_mutex = xSemaphoreCreateMutex();
}

void lump_command_push(const uint8_t raw[LUMP_PAYLOAD_LEN]) {
    lump_command_entry_t entry;
    entry.seq          = raw[0]; /* コマンド用パケットはbyte0=シーケンス番号 */
    entry.type         = (lump_sensor_type_t)((raw[1] >> 5) & 0x07);
    entry.command      = raw[1] & 0x1F;
    entry.instance_id  = raw[2]; /* コマンド用パケットはbyte2=インスタンスID */

    uint8_t lo, hi;
    lo = raw[3];  hi = raw[4];  entry.v1 = (int16_t)((hi << 8) | lo);
    lo = raw[5];  hi = raw[6];  entry.v2 = (int16_t)((hi << 8) | lo);
    lo = raw[7];  hi = raw[8];  entry.v3 = (int16_t)((hi << 8) | lo);
    lo = raw[9];  hi = raw[10]; entry.v4 = (int16_t)((hi << 8) | lo);

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_queue[s_head] = entry;
        s_head = (s_head + 1) % LUMP_COMMAND_QUEUE_CAPACITY;

        if (s_count < LUMP_COMMAND_QUEUE_CAPACITY) {
            s_count++;
        } else {
            /* 満杯なので、最も古いものを1つ捨てる(tailを進める) */
            s_tail = (s_tail + 1) % LUMP_COMMAND_QUEUE_CAPACITY;
        }
        xSemaphoreGive(s_mutex);
    }
}

bool lump_command_pop(lump_command_entry_t *out) {
    bool ok = false;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        if (s_count > 0) {
            *out = s_queue[s_tail];
            s_tail = (s_tail + 1) % LUMP_COMMAND_QUEUE_CAPACITY;
            s_count--;
            ok = true;
        }
        xSemaphoreGive(s_mutex);
    }
    return ok;
}

size_t lump_command_count(void) {
    size_t c;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        c = s_count;
        xSemaphoreGive(s_mutex);
    } else {
        c = 0;
    }
    return c;
}