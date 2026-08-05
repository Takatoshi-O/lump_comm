#pragma once

/*
 * LUMPプロトコル定数。
 * 一次情報: pybricks/technical-info/uart-protocol.md
 * および、実機検証で判明した追加の手順(ホストの0xF0検出パルス、
 * デバイス側の500ms Lowパルス、自分自身のACK送信など)。
 */

#define LUMP_MSG_SYS          0x00
#define LUMP_MSG_CMD          0x40
#define LUMP_MSG_INFO         0x80
#define LUMP_MSG_DATA         0xC0
#define LUMP_MSG_TYPE_MASK    0xC0
#define LUMP_MSG_LOWER_MASK   0x07

#define LUMP_BYTE_SYNC        0x00
#define LUMP_BYTE_NACK        0x02
#define LUMP_BYTE_ACK         0x04
#define LUMP_BYTE_DETECT      0xF0   /* ホストのポートスキャン信号 */

#define LUMP_CMD_TYPE         0x00
#define LUMP_CMD_MODES        0x01
#define LUMP_CMD_SPEED        0x02
#define LUMP_CMD_EXT_MODE     0x06
#define LUMP_CMD_VERSION      0x07

#define LUMP_INFO_NAME        0x00
#define LUMP_INFO_MAPPING     0x05
#define LUMP_INFO_FORMAT      0x80

#define LUMP_DATA8            0x00
#define LUMP_DATA16           0x01

#define LUMP_MODE_0           0x00

#define LUMP_SYNC_BAUD_US     416   /* 1/2400秒 ≒ 416us */
#define LUMP_RUN_BAUD         115200
