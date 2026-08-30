#pragma once
/**
 * @file lump_slots.h
 * @brief センサー種別ごとの送信用スロットを管理し、更新されたデータをラウンドロビン方式でパケット化する内部APIを定義します。
 */

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"

/*
 * センサー種別ごとに独立した保管場所(棚)を持たせることで、
 * 異なるセンサー種別同士が互いのデータを上書きしてしまう問題を防ぐ。
 * (例: カラーセンサータスクとカメラタスクが同時に
 *  lump_device_report() を呼んでも、互いに干渉しない)
 */

/** @brief センサー種別ごとの送信用スロットを初期化します。 */
void lump_slots_init(void);

/* 指定したセンサー種別の棚に書き込む(シーケンス番号は内部で自動的に+1する) */
/**
 * @brief 指定センサー種別の送信用スロットへ最新値を書き込みます。
 *
 * @param type センサー種別です。
 * @param mode モード番号です。
 * @param sensorID センサーインスタンスIDです。
 * @param v1 第1値です。
 * @param v2 第2値です。
 * @param v3 第3値です。
 * @param v4 第4値です。
 */
void lump_slots_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                        int16_t v1, int16_t v2, int16_t v3, int16_t v4);

/*
 * まだ送信されていない(dirtyな)棚を、ラウンドロビン順で1つ選び、
 * LUMP_PAYLOAD_LEN バイトのパケット形式にパックしてoutへ書き出す。
 * 戻り値: 送るべき新しいデータがあった場合 true。
 * 何も新しいデータがない場合は false を返す(呼び出し側は前回送った
 * 内容を再送するなど、ハートビート的な扱いをすること)。
 */
/**
 * @brief 未送信のスロットを1件選択し、送信用パケットへ詰めます。
 *
 * センサー種別をラウンドロビン順に選択します。
 *
 * @param out LUMP_PAYLOAD_LENバイトの出力先です。
 * @return 新しいデータを取得できた場合trueです。
 */
bool lump_slots_pick_next(uint8_t out[LUMP_PAYLOAD_LEN]);
