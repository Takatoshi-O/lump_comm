#include "lump_slots.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LUMP_SLOT_BUF_CAPACITY 16

typedef struct {
    lump_sensor_type_t type;
    uint8_t mode;
    uint8_t sensorID;
    int16_t values[4];
} lump_slot_entry_t;

/* instance_idごとの分割はやめ、全instance共通の
 * 16エントリ・ラウンドロビン式リングバッファに一本化 */
static lump_slot_entry_t s_buf[LUMP_SLOT_BUF_CAPACITY];
static uint8_t s_head = 0;          /* 次に書き込む位置 */
static uint8_t s_read_cursor = 0;   /* 次に読み出す位置(最古の未送信) */
static uint8_t s_count = 0;         /* バッファ内の未送信エントリ数 */
static SemaphoreHandle_t s_buf_mutex;

/* 全instance共通のシーケンス番号。ackを除き、実際にデータを
 * 送信する(pick_nextが呼ばれる)たびにインクリメントする */
static uint8_t s_seq_counter = 0;

void lump_slots_init(void) 
{
    memset(s_buf, 0, sizeof(s_buf));
    s_head = 0;
    s_read_cursor = 0;
    s_count = 0;
    s_seq_counter = 0;
    s_buf_mutex = xSemaphoreCreateMutex();
}

/* リングバッファへの新規エントリ追加(満杯時は最古のエントリを上書き) */
static void push_entry(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                        int16_t v1, int16_t v2, int16_t v3, int16_t v4) {
    if (xSemaphoreTake(s_buf_mutex, portMAX_DELAY) == pdTRUE) {
        lump_slot_entry_t *e = &s_buf[s_head];
        e->type = type;
        e->mode = mode & 0x1F; /* 下位5bitのみ使う */
        e->sensorID = sensorID;
        e->values[0] = v1;
        e->values[1] = v2;
        e->values[2] = v3;
        e->values[3] = v4;

        s_head = (uint8_t)((s_head + 1) % LUMP_SLOT_BUF_CAPACITY);

        if (s_count < LUMP_SLOT_BUF_CAPACITY) {
            s_count++;
        } else {
            /* 満杯だったので最古のエントリを上書きした
             * -> 読み出しカーソルもラウンドロビンで一つ進める */
            s_read_cursor = (uint8_t)((s_read_cursor + 1) % LUMP_SLOT_BUF_CAPACITY);
        }

        xSemaphoreGive(s_buf_mutex);
    }
}

void lump_slots_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                        int16_t v1, int16_t v2, int16_t v3, int16_t v4) 
{
    if (type >= LUMP_TYPE_MAX) return;
    push_entry(type, mode, sensorID, v1, v2, v3, v4);
}


/* entryの内容をパケット形式 (byte0=sequence, byte1=種別+モード, byte2=センサーID, byte3-10=int16x4) にパックする */
static void pack_entry(const lump_slot_entry_t *e, uint8_t seq, uint8_t out[LUMP_PAYLOAD_LEN]) 
{
    out[0] = seq;
    out[1] = (uint8_t)((e->type & 0x07) << 5) | (e->mode & 0x1F);
    out[2] = e->sensorID;
    for (int i = 0; i < 4; i++) {
        int16_t v = e->values[i];
        out[3 + i * 2]     = (uint8_t)(v & 0xFF);        /* リトルエンディアン */
        out[3 + i * 2 + 1] = (uint8_t)((v >> 8) & 0xFF);
    }
}

bool lump_slots_pick_next(uint8_t out[LUMP_PAYLOAD_LEN]) 
{
    bool ok = false;

    if (xSemaphoreTake(s_buf_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (s_count > 0) {
            lump_slot_entry_t *e = &s_buf[s_read_cursor];

            /* 実際にデータを送信するのでここでシーケンス番号を消費する
             * (ackハートビートはここを通らないためインクリメントされない) */
            uint8_t seq = s_seq_counter++;
            pack_entry(e, seq, out);

            s_read_cursor = (uint8_t)((s_read_cursor + 1) % LUMP_SLOT_BUF_CAPACITY);
            s_count--;
            ok = true;
        }
        xSemaphoreGive(s_buf_mutex);
    }

    return ok;
}