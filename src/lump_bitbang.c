#include "lump_bitbang.h"
#include "lump_protocol.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static int s_tx_gpio;
static int s_rx_gpio;

void lump_bb_init(int tx_gpio, int rx_gpio) {
    s_tx_gpio = tx_gpio;
    s_rx_gpio = rx_gpio;
    gpio_set_direction(s_tx_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(s_rx_gpio, GPIO_MODE_INPUT);
    gpio_set_level(s_tx_gpio, 1);
}

void lump_bb_set_tx_level(int level) {
    gpio_set_level(s_tx_gpio, level);
}

void lump_bb_send_byte(uint8_t dat) {
    /* スタートビット */
    gpio_set_level(s_tx_gpio, 0);
    esp_rom_delay_us(LUMP_SYNC_BAUD_US);

    /* データビット (LSBファースト) */
    uint8_t mask = 1;
    while (mask) {
        gpio_set_level(s_tx_gpio, (dat & mask) ? 1 : 0);
        esp_rom_delay_us(LUMP_SYNC_BAUD_US);
        mask <<= 1;
    }

    /* ストップビット */
    gpio_set_level(s_tx_gpio, 1);
    esp_rom_delay_us(LUMP_SYNC_BAUD_US);
}

void lump_bb_send_bytes(const uint8_t *buf, int len) {
    for (int i = 0; i < len; i++) {
        lump_bb_send_byte(buf[i]);
    }
}

unsigned int lump_bb_recv_byte(void) {
    unsigned long cnt = 0;
    while (gpio_get_level(s_rx_gpio) == 1) {
        cnt++;
        if (cnt > 2000000) return 0xFFFF;
    }
    /* スタートビットの中央まで待つ */
    esp_rom_delay_us(LUMP_SYNC_BAUD_US * 3 / 2);

    uint8_t dat = 0;
    uint8_t mask = 1;
    while (mask) {
        if (gpio_get_level(s_rx_gpio)) dat |= mask;
        esp_rom_delay_us(LUMP_SYNC_BAUD_US);
        mask <<= 1;
    }
    return dat;
}
