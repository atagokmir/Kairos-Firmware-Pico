#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define MZ80_PIN 15
#define DEBOUNCE_US 200000

volatile uint32_t last_rise_time = 0;
volatile uint32_t cycle_time_us = 0;
volatile bool new_cycle = false;

void gpio_callback(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_RISE)) return;
    uint32_t now = time_us_32();
    if ((now - last_rise_time) < DEBOUNCE_US) return;
    if (last_rise_time != 0) {
        cycle_time_us = now - last_rise_time;
        new_cycle = true;
    }
    last_rise_time = now;
}

int main() {
    stdio_init_all();
    gpio_init(MZ80_PIN);
    gpio_set_dir(MZ80_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(MZ80_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    while (true) {
        if (new_cycle) {
            new_cycle = false;
            uint32_t ct = cycle_time_us;
            printf("CYCLE %lu\n", ct);
        }
    }
}