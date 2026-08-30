#pragma once
/**
 * @file lump_bitbang.h
 * @brief LUMPハンドシェイクで使用する低速GPIOビットバンギング送受信の内部APIを定義します。
 */

#include <stdint.h>

/*
 * 2400bps専用のGPIOビットバンギング送受信。
 * ESP32のハードウェアUARTペリフェラルは2400bpsのような低速を
 * 正確に扱えないため、ハンドシェイクフェーズだけはこちらを使う。
 * (115200bps以降のDATAフェーズは通常のハードウェアUARTを使う)
 */

/**
 * @brief ビットバンギング送受信に使用するGPIOを初期化します。
 *
 * @param tx_gpio 送信GPIO番号です。
 * @param rx_gpio 受信GPIO番号です。
 */
void lump_bb_init(int tx_gpio, int rx_gpio);

/**
 * @brief 2400bpsのビットバンギングで1バイト送信します。
 *
 * @param dat 送信する1バイトです。
 */
void lump_bb_send_byte(uint8_t dat);
/**
 * @brief 2400bpsのビットバンギングで複数バイト送信します。
 *
 * @param buf 送信データです。
 * @param len 送信バイト数です。
 */
void lump_bb_send_bytes(const uint8_t *buf, int len);

/* 戻り値: 0-255=受信データ, 0xFFFF=タイムアウト */
/**
 * @brief 2400bpsのビットバンギングで1バイト受信します。
 *
 * @return 0x00～0xFFの受信データ、または0xFFFFのタイムアウトです。
 */
unsigned int lump_bb_recv_byte(void);

/**
 * @brief TX GPIOの論理レベルを直接設定します。
 *
 * @param level 出力レベルです。
 */
void lump_bb_set_tx_level(int level);
