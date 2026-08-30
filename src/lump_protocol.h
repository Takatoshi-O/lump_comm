#pragma once
/**
 * @file lump_protocol.h
 * @brief LUMPプロトコルで使用するメッセージ種別、制御コード、コマンド、データ形式、通信速度などの定数を定義します。
 */

/*
 * LUMPプロトコル定数。
 * 一次情報: pybricks/technical-info/uart-protocol.md
 * および、実機検証で判明した追加の手順(ホストの0xF0検出パルス、
 * デバイス側の500ms Lowパルス、自分自身のACK送信など)。
 */

/** @brief LUMPシステムメッセージの種別値です。 */
#define LUMP_MSG_SYS          0x00
/** @brief LUMPコマンドメッセージの種別値です。 */
#define LUMP_MSG_CMD          0x40
/** @brief LUMP情報メッセージの種別値です。 */
#define LUMP_MSG_INFO         0x80
/** @brief LUMPデータメッセージの種別値です。 */
#define LUMP_MSG_DATA         0xC0
#define LUMP_MSG_TYPE_MASK    0xC0
#define LUMP_MSG_LOWER_MASK   0x07

/** @brief LUMP同期コードです。 */
#define LUMP_BYTE_SYNC        0x00
/** @brief LUMP NACK制御コードです。 */
#define LUMP_BYTE_NACK        0x02
/** @brief LUMP ACK制御コードです。 */
#define LUMP_BYTE_ACK         0x04
/** @brief SPIKEホストからのデバイス検出用制御コードです。 */
#define LUMP_BYTE_DETECT      0xF0   /* ホストのポートスキャン信号 */

/** @brief デバイスタイプ設定コマンドです。 */
#define LUMP_CMD_TYPE         0x00
/** @brief モード情報取得コマンドです。 */
#define LUMP_CMD_MODES        0x01
/** @brief 通信速度設定コマンドです。 */
#define LUMP_CMD_SPEED        0x02
/** @brief 拡張モード情報取得コマンドです。 */
#define LUMP_CMD_EXT_MODE     0x06
/** @brief プロトコルバージョン取得コマンドです。 */
#define LUMP_CMD_VERSION      0x07

/** @brief デバイス名情報の種別値です。 */
#define LUMP_INFO_NAME        0x00
/** @brief データマッピング情報の種別値です。 */
#define LUMP_INFO_MAPPING     0x05
/** @brief モードフォーマット情報の種別値です。 */
#define LUMP_INFO_FORMAT      0x80

/** @brief 8bitデータ形式の種別値です。 */
#define LUMP_DATA8            0x00
/** @brief 16bitデータ形式の種別値です。 */
#define LUMP_DATA16           0x01

/** @brief LUMPの予約モード0を表す定数です。 */
#define LUMP_MODE_0           0x00

/** @brief 2400bps同期通信で1ビットに相当するおおよその待ち時間(μs)です。 */
#define LUMP_SYNC_BAUD_US     416   /* 1/2400秒 ≒ 416us */
/** @brief データフェーズで使用するUART通信速度です。 */
#define LUMP_RUN_BAUD         115200
