#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"

/*
 * センサー種別ごとに独立した保管場所(棚)を持たせることで、
 * 異なるセンサー種別同士が互いのデータを上書きしてしまう問題を防ぐ。
 * (例: カラーセンサータスクとカメラタスクが同時に
 *  lump_device_report() を呼んでも、互いに干渉しない)
 */

void lump_slots_init(void);

/* 指定したセンサー種別の棚に書き込む(シーケンス番号は内部で自動的に+1する) */
void lump_slots_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                        int16_t v1, int16_t v2, int16_t v3, int16_t v4);

/* 指定したセンサー種別の棚に書き込む(シーケンス番号を任意に設定する) */
void lump_slots_calib(lump_sensor_type_t type, uint8_t mode, uint8_t sequence, uint8_t sensorID,
                        int16_t v1, int16_t v2, int16_t v3, int16_t v4);


/*
 * まだ送信されていない(dirtyな)棚を、ラウンドロビン順で1つ選び、
 * LUMP_PAYLOAD_LEN バイトのパケット形式にパックしてoutへ書き出す。
 * 戻り値: 送るべき新しいデータがあった場合 true。
 * 何も新しいデータがない場合は false を返す(呼び出し側は前回送った
 * 内容を再送するなど、ハートビート的な扱いをすること)。
 */
bool lump_slots_pick_next(uint8_t out[LUMP_PAYLOAD_LEN]);
