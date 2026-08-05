#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== ピン・デバイス設定 ===================== */
/* 実際の配線・デバイスIDに合わせてこの値を変更してください */
#define LUMP_GPIO_TX          ((gpio_num_t)CONFIG_LUMP_TX_GPIO)
#define LUMP_GPIO_RX          ((gpio_num_t)CONFIG_LUMP_RX_GPIO)
#define LUMP_DEVICE_TYPE_ID   80
#define LUMP_MODE_NAME        "DATA"

/*
 * 1メッセージのデータ長(バイト数)。
 * 現在のパケット構造(10バイト)を前提にしている:
 *   byte0:   [センサー種別:3bit][モード:5bit]
 *   byte1:   センサーID
 *   byte2:   シーケンス番号
 *   byte3-4: value1 (int16, リトルエンディアン)
 *   byte5-6: value2 (int16, リトルエンディアン)
 *   byte7-8: value3 (int16, リトルエンディアン)
 *   byte9-10: value4 (int16, リトルエンディアン)
 */
#define LUMP_PAYLOAD_LEN      11

/* センサー種別(byte0の上位3bit, 0〜7)。プロジェクトに合わせて追加してよい */
typedef enum {
    LUMP_SYSTEM  = 0,
    LUMP_TYPE_1  = 1,
    LUMP_TYPE_2  = 2,
    LUMP_TYPE_3  = 3,
    LUMP_TYPE_4  = 4,
    LUMP_TYPE_5  = 5,
    LUMP_TYPE_6  = 6,
    LUMP_TYPE_7  = 7, /* 将来のプロトコル拡張用に予約 */
    LUMP_TYPE_MAX,
} lump_sensor_type_t;

/* ===================== 公開API ===================== */

/*
 * バックグラウンドタスクを起動する。app_main() から1回だけ呼ぶこと。
 * 内部でハンドシェイク〜データフェーズのループを別タスクとして開始する。
 */
void lump_device_start(void);

/* 現在SPIKE側との接続(DATAフェーズ)が確立しているかどうか */
bool lump_device_is_connected(void);

/*
 * センサー処理タスクから呼ぶ。指定したセンサー種別の棚にだけ書き込むので、
 * 別のセンサー種別のデータを上書きすることはない。
 * mode: そのセンサー種別内でのサブモード(0〜31, 例: 検出/未検出の状態など)
 */
void lump_device_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                         int16_t v1, int16_t v2, int16_t v3, int16_t v4);

/*
 * SPIKEから最後に書き込まれた値(コマンド)を読み出す。
 * out には LUMP_PAYLOAD_LEN バイト分の領域を渡すこと。
 */
void lump_device_get_command(uint8_t out[LUMP_PAYLOAD_LEN]);

#ifdef __cplusplus
}
#endif
