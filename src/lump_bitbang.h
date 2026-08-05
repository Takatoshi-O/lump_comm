#pragma once

#include <stdint.h>

/*
 * 2400bps専用のGPIOビットバンギング送受信。
 * ESP32のハードウェアUARTペリフェラルは2400bpsのような低速を
 * 正確に扱えないため、ハンドシェイクフェーズだけはこちらを使う。
 * (115200bps以降のDATAフェーズは通常のハードウェアUARTを使う)
 */

void lump_bb_init(int tx_gpio, int rx_gpio);

void lump_bb_send_byte(uint8_t dat);
void lump_bb_send_bytes(const uint8_t *buf, int len);

/* 戻り値: 0-255=受信データ, 0xFFFF=タイムアウト */
unsigned int lump_bb_recv_byte(void);

void lump_bb_set_tx_level(int level);
