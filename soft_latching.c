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

#include <stdio.h>
#include "ch32fun.h"

uint8_t  button_pin  = PC7;      // Latch and button pin
uint8_t  blink_pin   = PD5;      // LED pin
uint8_t  blink_state = FUN_LOW;  // LED on
uint32_t blink_delay = 500;      // LED blink delay

void systick_init(void)
{
    SysTick->CTLR = 0;
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CMP  = FUNCONF_SYSTEM_CORE_CLOCK / 1000 * blink_delay;
    SysTick->CNT  = 0;
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
}

void disable_systick(void)
{
    NVIC_DisableIRQ(SysTicK_IRQn);
}

void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    // SysTick maintains accurate 20ms period
    SysTick->CMP += FUNCONF_SYSTEM_CORE_CLOCK / 1000 * blink_delay;
    SysTick->SR = 0;

    // blink the LED
    funDigitalWrite(blink_pin, blink_state);
    blink_state = (blink_state == FUN_LOW) ? FUN_HIGH : FUN_LOW;
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
    funPinMode(blink_pin, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);

    systick_init();

#define STATE_WAIT_FOR_KEY_PRESS   0
#define STATE_KEY_PRESS_DEBOUNCE   1
#define STATE_WAIT_FOR_KEY_RELEASE 2
#define STATE_KEY_RELEASE_DEBOUNCE 3
#define DEBOUNCE_INTERVAL          5  // 5ms
#define DEBOUNCE_STABLE_CYCLES     2  // 5ms x 2 = 10ms
#define KEY_OFF                    0
#define KEY_ON                     1

    uint8_t key_debounce_state = STATE_WAIT_FOR_KEY_PRESS;
    uint8_t key_last_status    = KEY_OFF;
    uint8_t key_cycle_count    = 0;
    // uint8_t key_pressed        = 0;
    uint8_t key_released = 0;

    uint8_t led_state = 0;

    // If the key is holding after power on, wait until it is released.
    // TODO: There should be a more elegant way...
    if (funDigitalRead(button_pin) == FUN_LOW)
    {
        while (1)
        {
            if (funDigitalRead(button_pin) == FUN_HIGH)
            {
                Delay_Ms(DEBOUNCE_INTERVAL);
                if (funDigitalRead(button_pin) == FUN_HIGH)
                {
                    break;
                }
            }
            Delay_Ms(DEBOUNCE_INTERVAL);
        }
    }

    // Key states
    // TODO: Create a library
    //  +----------------------------+         +----------------------------+
    //  |  STATE_WAIT_FOR_KEY_PRESS  | <-----> |  STATE_KEY_PRESS_DEBOUNCE  |
    //  +----------------------------+         +----------------------------+
    //                ^                                      |
    //                |                                      V
    //  +----------------------------+         +----------------------------+
    //  | STATE_KEY_RELEASE_DEBOUNCE | <-----> | STATE_WAIT_FOR_KEY_RELEASE |
    //  +----------------------------+         +----------------------------+
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
                        // key_pressed        = 1;
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

        // if (key_pressed)
        // {
        //     disable_systick();
        //     funDigitalWrite(blink_pin, FUN_LOW);
        // }

        if (key_released)
        {
            led_state++;
            blink_delay = (blink_delay == 500) ? 100 : 500;
            // printf("Key Released. LED status = %d, blink_delay = %d\n", led_state, blink_delay);
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

        Delay_Ms(DEBOUNCE_INTERVAL);
    }
}
