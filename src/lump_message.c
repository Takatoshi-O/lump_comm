#include "lump_message.h"
#include "lump_protocol.h"

static uint8_t length_code(uint8_t len, uint8_t *padded_len) {
    if (len <= 1)       { *padded_len = 1;  return 0x00; }
    else if (len <= 2)  { *padded_len = 2;  return 0x08; }
    else if (len <= 4)  { *padded_len = 4;  return 0x10; }
    else if (len <= 8)  { *padded_len = 8;  return 0x18; }
    else if (len <= 16) { *padded_len = 16; return 0x20; }
    else                { *padded_len = 32; return 0x28; }
}

int lump_msg_build_cmd_or_data(uint8_t *out, uint8_t msg_type, uint8_t cmd_or_mode,
                                const uint8_t *data, uint8_t data_len) {
    uint8_t padded_len;
    uint8_t code = length_code(data_len, &padded_len);
    uint8_t header = msg_type | code | cmd_or_mode;
    uint8_t checksum = 0xFF ^ header;
    int idx = 0;

    out[idx++] = header;
    for (int i = 0; i < padded_len; i++) {
        uint8_t b = (i < data_len) ? data[i] : 0x00;
        checksum ^= b;
        out[idx++] = b;
    }
    out[idx++] = checksum;
    return idx;
}

int lump_msg_build_info(uint8_t *out, uint8_t mode, uint8_t info_type,
                         const uint8_t *data, uint8_t data_len) {
    uint8_t padded_len;
    uint8_t code = length_code(data_len, &padded_len);
    uint8_t header = LUMP_MSG_INFO | code | mode;
    uint8_t checksum = 0xFF ^ header ^ info_type;
    int idx = 0;

    out[idx++] = header;
    out[idx++] = info_type;
    for (int i = 0; i < padded_len; i++) {
        uint8_t b = (i < data_len) ? data[i] : 0x00;
        checksum ^= b;
        out[idx++] = b;
    }
    out[idx++] = checksum;
    return idx;
}

uint8_t lump_msg_payload_len_from_header(uint8_t header) {
    uint8_t size_code = header & 0x38;
    switch (size_code) {
        case 0x00: return 1;
        case 0x08: return 2;
        case 0x10: return 4;
        case 0x18: return 8;
        case 0x20: return 16;
        default:   return 32;
    }
}

uint8_t lump_msg_checksum(uint8_t header, const uint8_t *payload, uint8_t len) {
    uint8_t checksum = 0xFF ^ header;
    for (int i = 0; i < len; i++) {
        checksum ^= payload[i];
    }
    return checksum;
}
