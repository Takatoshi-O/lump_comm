#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lump_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SPIKEから送られてくるコマンドパケットを受信し、直近16件をリングバッファで
 * 保持するモジュール。17件目が来ると、最も古い未処理のものから捨てられる。
 *
 * コマンド用パケットは、通常のセンサーイベント用パケットと byte1/byte2 の
 * 意味が入れ替わっている点に注意:
 *   byte0:    [センサー種別:3bit][コマンド番号:5bit]
 *   byte1:    シーケンス番号
 *   byte2:    インスタンスID
 *   byte3-10: int16 x4
 */

#define LUMP_COMMAND_QUEUE_CAPACITY 16

typedef struct {
    lump_sensor_type_t type;
    uint8_t command;      /* byte0下位5bit */
    uint8_t seq;
    uint8_t instance_id;
    int16_t v1, v2, v3, v4;
} lump_command_entry_t;

void lump_command_init(void);

/*
 * data_phase() 内で、SPIKEからの書き込みメッセージを受け取るたびに呼ぶ。
 * (lump_comm.c 側で判定してから呼び分けること)。
 */
void lump_command_push(const uint8_t raw[LUMP_PAYLOAD_LEN]);

/*
 * 最も古い未処理のコマンドを1件取り出す(取り出すとキューから消費される)。
 * 戻り値: 取得できれば true、キューが空なら false。
 */
bool lump_command_pop(lump_command_entry_t *out);

/* 現在キューに溜まっている件数 */
size_t lump_command_count(void);

#ifdef __cplusplus
}
#endif