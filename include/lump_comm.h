#pragma once
/**
 * @file lump_comm.h
 * @brief LEGO SPIKEとのLUMP通信を利用するための公開API、センサー種別、通信パケットの基本設定を定義します。
 */

#include <stdint.h>
#include <stdbool.h>

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== ピン・デバイス設定 ===================== */
/* 実際の配線・デバイスIDに合わせてこの値を変更してください */
/** @brief Kconfigで指定されたLUMP通信TX GPIOです。 */
#define LUMP_GPIO_TX          ((gpio_num_t)CONFIG_LUMP_TX_GPIO)
/** @brief Kconfigで指定されたLUMP通信RX GPIOです。 */
#define LUMP_GPIO_RX          ((gpio_num_t)CONFIG_LUMP_RX_GPIO)
/** @brief SPIKEへ通知するこのデバイスのLUMPデバイスタイプIDです。 */
#define LUMP_DEVICE_TYPE_ID   80
/** @brief デバイス情報として使用するモード名です。 */
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
/** @brief センサーデータパケットの固定ペイロード長です。 */
#define LUMP_PAYLOAD_LEN      11

/* センサー種別(byte0の上位3bit, 0〜7)。プロジェクトに合わせて追加してよい */
/**
 * @brief LUMPパケットで使用するセンサー種別IDです。
 *
 * 上位ビット側のセンサー種別と各センサーAPIの分類に使用されます。
 */
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
/**
 * @brief LUMP通信のバックグラウンドタスクを起動します。
 *
 * 通常はapp_main()から1回だけ呼び出します。
 */
void lump_device_start(void);

/* 現在SPIKE側との接続(DATAフェーズ)が確立しているかどうか */
/**
 * @brief SPIKEとのデータ通信状態を取得します。
 *
 * @return 接続が確立している場合true、それ以外はfalseです。
 */
bool lump_device_is_connected(void);

/*
 * センサー処理タスクから呼ぶ。指定したセンサー種別の棚にだけ書き込むので、
 * 別のセンサー種別のデータを上書きすることはない。
 * mode: そのセンサー種別内でのサブモード(0〜31, 例: 検出/未検出の状態など)
 */
/**
 * @brief センサー値を送信待ちスロットへ登録します。
 *
 * @param type センサー種別です。
 * @param mode センサー内のモード番号です。
 * @param sensorID センサーインスタンスIDです。
 * @param v1 第1値です。
 * @param v2 第2値です。
 * @param v3 第3値です。
 * @param v4 第4値です。
 */
void lump_device_report(lump_sensor_type_t type, uint8_t mode, uint8_t sensorID,
                         int16_t v1, int16_t v2, int16_t v3, int16_t v4);

#ifdef __cplusplus
}
#endif
