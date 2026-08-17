#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/rand.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#define DUST_PIN           20
#define SUBCYCLE_MS        1000  // ventana dust; pot se relee cada subciclo
#define MAX_CLICKS_PER_SEC 4     // 4 clicks/s máx = ~40 en 10 s
#define LED_PERIOD_MS      500

static bool     led_state   = false;
static uint32_t led_last_ms = 0;
static uint     g_slice;
static uint     g_channel;

static void update_led(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - led_last_ms >= LED_PERIOD_MS) {
        led_state = !led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
        led_last_ms = now;
    }
}

// Actualiza el brillo del LED con la lectura actual del pot
static void update_pwm(void) {
    pwm_set_chan_level(g_slice, g_channel, adc_read());
}

// sleep que mantiene LED y PWM actualizados en chunks de 10ms
static void blink_sleep_ms(uint32_t ms) {
    uint32_t end = to_ms_since_boot(get_absolute_time()) + ms;
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now >= end) break;
        uint32_t chunk = end - now;
        if (chunk > 10) chunk = 10;
        sleep_ms(chunk);
        update_led();
        update_pwm();
    }
}

static void sort_times(uint32_t *arr, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (arr[j] < arr[i]) {
                uint32_t t = arr[i]; arr[i] = arr[j]; arr[j] = t;
            }
}

int main() {
    if (cyw43_arch_init()) return -1;

    // ADC en GP26 (ADC0)
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    // PWM en GP18
    gpio_set_function(18, GPIO_FUNC_PWM);
    g_slice   = pwm_gpio_to_slice_num(18);
    g_channel = pwm_gpio_to_channel(18);
    pwm_set_wrap(g_slice, 4095);
    pwm_set_chan_level(g_slice, g_channel, 0);
    pwm_set_enabled(g_slice, true);

    // Dust en GP20
    gpio_init(DUST_PIN);
    gpio_set_dir(DUST_PIN, GPIO_OUT);

    while (true) {
        // Releer pot cada 1 segundo para actualizar n_clicks
        uint16_t adc_val = adc_read();
        int n = (int)((uint32_t)adc_val * MAX_CLICKS_PER_SEC / 4095);

        if (n == 0) {
            blink_sleep_ms(SUBCYCLE_MS);
            continue;
        }

        // Generar n tiempos aleatorios dentro del subciclo y ordenarlos
        uint32_t times[MAX_CLICKS_PER_SEC];
        for (int i = 0; i < n; i++)
            times[i] = get_rand_32() % (SUBCYCLE_MS - 50);
        sort_times(times, n);

        uint32_t t0 = to_ms_since_boot(get_absolute_time());

        for (int i = 0; i < n; i++) {
            uint32_t now = to_ms_since_boot(get_absolute_time()) - t0;
            if (times[i] <= now) continue; // el pulso anterior lo consumió

            blink_sleep_ms(times[i] - now);

            uint32_t dur = 15 + get_rand_32() % 36; // 15-50 ms
            gpio_put(DUST_PIN, 1);
            sleep_ms(dur); // sleep preciso para el ancho del pulso
            gpio_put(DUST_PIN, 0);
        }

        // Esperar el resto del subciclo
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - t0;
        if (elapsed < SUBCYCLE_MS)
            blink_sleep_ms(SUBCYCLE_MS - elapsed);
    }
}
