#pragma once
/**
 * @file lump_message.h
 * @brief LUMPプロトコルのCMD/DATA/INFOメッセージを組み立て、ヘッダーからペイロード長を求め、チェックサムを計算する内部APIを定義します。
 */

#include <stdint.h>
#include <stdbool.h>

/*
 * LUMPメッセージの組み立て・検証。
 * g_msgbuf 相当のバッファはこのモジュール内に隠蔽し、呼び出し側には
 * 「組み立てた結果を送信バイト数として返す」形で公開する。
 */

/** @brief LUMPメッセージ組み立て用出力バッファに必要な最大サイズです。 */
#define LUMP_MSGBUF_MAX 64

/* CMD/DATAメッセージを組み立てる。out には LUMP_MSGBUF_MAX バイト以上を渡すこと。
 * 戻り値: 実際に組み立てたバイト数 */
/**
 * @brief CMDまたはDATA形式のLUMPメッセージを組み立てます。
 *
 * @param out 出力バッファです。LUMP_MSGBUF_MAX以上の容量を用意します。
 * @param msg_type メッセージ種別です。
 * @param cmd_or_mode コマンドまたはモード値です。
 * @param data ペイロードデータです。
 * @param data_len ペイロード長です。
 * @return 組み立てたメッセージの実バイト数です。
 */
int lump_msg_build_cmd_or_data(uint8_t *out, uint8_t msg_type, uint8_t cmd_or_mode,
                                const uint8_t *data, uint8_t data_len);

/* INFOメッセージを組み立てる(info_typeバイトは長さフィールドの外側に付く) */
/**
 * @brief INFO形式のLUMPメッセージを組み立てます。
 *
 * @param out 出力バッファです。
 * @param mode 対象モードです。
 * @param info_type INFO種別です。
 * @param data ペイロードデータです。
 * @param data_len ペイロード長です。
 * @return 組み立てたメッセージの実バイト数です。
 */
int lump_msg_build_info(uint8_t *out, uint8_t mode, uint8_t info_type,
                         const uint8_t *data, uint8_t data_len);

/*
 * ヘッダーバイトから、後続のペイロード長(バイト数)を求める。
 * (BYTE_SYNC/NACK/ACK等の1バイト制御コードは対象外)
 */
/**
 * @brief メッセージヘッダーから後続ペイロード長を算出します。
 *
 * 制御用の1バイトコードには使用しません。
 *
 * @param header メッセージヘッダーです。
 * @return ペイロード長です。
 */
uint8_t lump_msg_payload_len_from_header(uint8_t header);

/* 0xFF ^ header ^ (payload全バイトのXOR) を計算する */
/**
 * @brief LUMPメッセージのXORベースチェックサムを計算します。
 *
 * @param header ヘッダーバイトです。
 * @param payload ペイロードです。
 * @param len ペイロード長です。
 * @return 計算されたチェックサムです。
 */
uint8_t lump_msg_checksum(uint8_t header, const uint8_t *payload, uint8_t len);
