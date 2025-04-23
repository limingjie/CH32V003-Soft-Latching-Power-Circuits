/*
 * CH32V003J4M6 (SOP-8) Soft Latching
 *
 * Reference:
 *  - https://circuitcellar.com/resources/quickbits/soft-latching-power-circuits/
 *  - https://github.com/cnlohr/ch32v003fun
 *
 * Aug 2023 by Li Mingjie
 *  - Email:  limingjie@outlook.com
 *  - GitHub: https://github.com/limingjie/
 */

#include "ch32fun.h"
// #include "ch32v003_GPIO_branchless.h"
#include <stdio.h>

// Pins
uint16_t led_pin    = PC1;
uint16_t button_pin = PD0;

int delay = 500;

void systick_init(void)
{
    SysTick->CTLR = 0;
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CMP  = FUNCONF_SYSTEM_CORE_CLOCK / 1000 * (delay * 2);  // delay x 2
    SysTick->CNT  = 0;
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
}

void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    // SysTick maintains accurate 20ms period
    SysTick->CMP += FUNCONF_SYSTEM_CORE_CLOCK / 1000 * (delay * 2);  // delay x 2
    SysTick->SR = 0;

    // blink the LED
    funDigitalWrite(led_pin, FUN_HIGH);
    Delay_Ms(delay);
    funDigitalWrite(led_pin, FUN_LOW);
}

int main()
{
    SystemInit();
    Delay_Ms(100);

    funGpioInitAll();
    // Power on latch
    funPinMode(button_pin, GPIO_CFGLR_IN_PUPD);
    funDigitalWrite(button_pin, FUN_HIGH);  // Input pull-up

    // Init LED pin
    funPinMode(led_pin, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);

    systick_init();

#define STATE_WAIT_FOR_KEY_PRESS   0
#define STATE_KEY_PRESS_DEBOUNCE   1
#define STATE_WAIT_FOR_KEY_RELEASE 2
#define STATE_KEY_RELEASE_DEBOUNCE 3
#define DEBOUNCE_CHECK_INTERVAL    5  // 5ms
#define DEBOUNCE_STABLE_CYCLES     2  // 5ms x 2 = 10ms
#define KEY_OFF                    0
#define KEY_ON                     1

    int key_debounce_state = STATE_WAIT_FOR_KEY_PRESS;
    int key_last_status    = KEY_OFF;
    int key_cycle_count    = 0;
    int key_released       = 0;

    int led_state = 0;

    // Wait key release after power on.
    if (funDigitalRead(button_pin) == FUN_LOW)
    {
        while (1)
        {
            if (funDigitalRead(button_pin) == FUN_HIGH)
            {
                break;
            }
            Delay_Ms(DEBOUNCE_CHECK_INTERVAL);
        }
    }

    // Key states
    //  STATE_WAIT_FOR_KEY_PRESS  <------->  STATE_KEY_PRESS_DEBOUNCE
    //             ^                                    |
    //             |                                    |
    //             |                                    V
    // STATE_KEY_RELEASE_DEBOUNCE <-------> STATE_WAIT_FOR_KEY_RELEASE
    while (1)
    {
        switch (key_debounce_state)
        {
            case STATE_WAIT_FOR_KEY_PRESS:
                if (funDigitalRead(button_pin) == FUN_LOW)  // pressed
                {
                    key_last_status    = KEY_ON;
                    key_cycle_count    = 0;
                    key_debounce_state = STATE_KEY_PRESS_DEBOUNCE;
                    // printf("STATE_KEY_PRESS_DEBOUNCE\n");
                }
                break;
            case STATE_KEY_PRESS_DEBOUNCE:
                ++key_cycle_count;
                if (key_last_status == KEY_ON && funDigitalRead(button_pin) == FUN_LOW)  // still pressed
                {
                    if (key_cycle_count >= DEBOUNCE_STABLE_CYCLES)
                    {
                        key_debounce_state = STATE_WAIT_FOR_KEY_RELEASE;
                        // printf("STATE_WAIT_FOR_KEY_RELEASE\n");
                    }
                }
                else
                {
                    key_debounce_state = STATE_WAIT_FOR_KEY_PRESS;
                    // printf("STATE_WAIT_FOR_KEY_PRESS\n");
                }
                break;
            case STATE_WAIT_FOR_KEY_RELEASE:
                if (funDigitalRead(button_pin) == FUN_HIGH)  // released
                {
                    // key_released       = 1;
                    // key_debounce_state = STATE_WAIT_FOR_KEY_PRESS;

                    key_last_status    = KEY_OFF;
                    key_cycle_count    = 0;
                    key_debounce_state = STATE_KEY_RELEASE_DEBOUNCE;
                    // printf("STATE_KEY_RELEASE_DEBOUNCE\n");
                }
                break;
            case STATE_KEY_RELEASE_DEBOUNCE:
                ++key_cycle_count;
                if (key_last_status == KEY_OFF && funDigitalRead(button_pin) == FUN_HIGH)  // still pressed
                {
                    if (key_cycle_count >= DEBOUNCE_STABLE_CYCLES)
                    {
                        key_released       = 1;
                        key_debounce_state = STATE_WAIT_FOR_KEY_PRESS;
                        // printf("STATE_WAIT_FOR_KEY_PRESS\n");
                    }
                }
                else
                {
                    key_debounce_state = STATE_WAIT_FOR_KEY_RELEASE;
                    // printf("STATE_WAIT_FOR_KEY_RELEASE\n");
                }
                break;
        }

        if (key_released)
        {
            led_state++;
            delay = (delay == 500) ? 100 : 500;
            // printf("Key Released. LED status = %d, delay = %d\n", led_state, delay);
            if (led_state > 8)
            {
                funDigitalWrite(button_pin, FUN_LOW);  // Input pull-down
            }
            else
            {
                systick_init();  // Re-initiate systick, change LED blink interval.
            }
            key_released = 0;
        }

        Delay_Ms(DEBOUNCE_CHECK_INTERVAL);
    }
}
