#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#define MZ80_PIN 15
#define DEBOUNCE_US 200000

volatile uint32_t last_rise_time = 0;
volatile uint32_t cycle_time_us = 0;
volatile bool new_cycle = false;
volatile bool started = false;

static char buf[8];
static int idx = 0;

void gpio_callback(uint gpio, uint32_t events) {
    if (!started) return;
    if (!(events & GPIO_IRQ_EDGE_RISE)) return;
    uint32_t now = time_us_32();
    if ((now - last_rise_time) < DEBOUNCE_US) return;
    if (last_rise_time != 0) {
        cycle_time_us = now - last_rise_time;
        new_cycle = true;
    }
    last_rise_time = now;
}

void usb_rx_callback(void *param) {
    int c;
    while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        if (c == '\n') {
            buf[idx] = '\0';
            if (strcmp(buf, "START") == 0) {
                started = true;
                last_rise_time = 0;
                new_cycle = false;
            } else if (strcmp(buf, "STOP") == 0) {
                started = false;
                last_rise_time = 0;
                new_cycle = false;
            }
            idx = 0;
        } else if (idx < 7) {
            buf[idx++] = (char)c;
        }
    }
}

int main() {
    stdio_init_all();
    gpio_init(MZ80_PIN);
    gpio_set_dir(MZ80_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(MZ80_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    stdio_set_chars_available_callback(usb_rx_callback, NULL);

    while (true) {
        if (new_cycle && started) {
            new_cycle = false;
            uint32_t ct = cycle_time_us;
            printf("CYCLE %lu\n", ct);
        }
        __wfi();  // interrupt gelene kadar uyu, CPU yanmaz
    }
}