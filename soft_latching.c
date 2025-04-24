/*
 * CH32V003 Soft Latching
 *
 * Reference:
 *  - https://circuitcellar.com/resources/quickbits/soft-latching-power-circuits/
 *  - https://github.com/cnlohr/ch32v003fun
 *
 * Apr 2025 by Li Mingjie
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

    // If the key is holding after power on, wait until it is released.
    // TODO: There should be a more elegant way...
    if (funDigitalRead(button_pin) == FUN_LOW)
    {
        while (1)
        {
            if (funDigitalRead(button_pin) == FUN_HIGH)
            {
                Delay_Ms(5);
                if (funDigitalRead(button_pin) == FUN_HIGH)
                {
                    break;
                }
            }
            Delay_Ms(5);
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

    enum debounce_states
    {
        STATE_WAIT_FOR_KEY_PRESS,
        STATE_KEY_PRESS_DEBOUNCE,
        STATE_WAIT_FOR_KEY_RELEASE,
        STATE_KEY_RELEASE_DEBOUNCE
    };

    enum key_states
    {
        KEY_UP,
        KEY_DOWN
    };

    enum key_events
    {
        KEY_NONE,
        KEY_PRESS,
        KEY_RELEASE,
        KEY_LONG_PRESS_RELEASE
    };

#define DEBOUNCE_INTERVAL      5    // 5ms
#define DEBOUNCE_STABLE_CYCLES 5    // 5ms x 5 = 25ms
#define LONG_PRESS_CYCLES      100  // 5ms x 100 = 500ms

    uint8_t  debounce_state           = STATE_WAIT_FOR_KEY_PRESS;
    uint8_t  key_event                = KEY_NONE;
    uint8_t  key_long_press           = 0;
    uint16_t key_cycle_count          = 0;
    uint16_t key_debounce_cycle_count = 0;

    while (1)
    {
        uint8_t key_state = (funDigitalRead(button_pin) == FUN_LOW) ? KEY_DOWN : KEY_UP;
        switch (debounce_state)
        {
            case STATE_WAIT_FOR_KEY_PRESS:
                if (key_state == KEY_DOWN)  // pressed
                {
                    key_debounce_cycle_count = 0;
                    debounce_state           = STATE_KEY_PRESS_DEBOUNCE;
                    printf("STATE_KEY_PRESS_DEBOUNCE\n");
                }
                break;
            case STATE_KEY_PRESS_DEBOUNCE:
                ++key_debounce_cycle_count;
                if (key_state == KEY_DOWN)  // still pressed
                {
                    if (key_debounce_cycle_count >= DEBOUNCE_STABLE_CYCLES)
                    {
                        key_event       = KEY_PRESS;
                        key_cycle_count = 0;
                        key_long_press  = 0;
                        debounce_state  = STATE_WAIT_FOR_KEY_RELEASE;
                        printf("STATE_WAIT_FOR_KEY_RELEASE\n");
                    }
                }
                else
                {
                    debounce_state = STATE_WAIT_FOR_KEY_PRESS;
                    printf("STATE_WAIT_FOR_KEY_PRESS\n");
                }
                break;
            case STATE_WAIT_FOR_KEY_RELEASE:
                ++key_cycle_count;
                if (key_state == KEY_UP)  // released
                {
                    if (key_cycle_count >= LONG_PRESS_CYCLES)
                    {
                        key_long_press = 1;
                        printf("Long Press!!! cycle = %d\n", key_cycle_count);
                    }
                    key_debounce_cycle_count = 0;
                    debounce_state           = STATE_KEY_RELEASE_DEBOUNCE;
                    printf("STATE_KEY_RELEASE_DEBOUNCE\n");
                }
                break;
            case STATE_KEY_RELEASE_DEBOUNCE:
                ++key_debounce_cycle_count;
                if (key_state == KEY_UP)  // still released
                {
                    if (key_debounce_cycle_count >= DEBOUNCE_STABLE_CYCLES)
                    {
                        key_event      = key_long_press ? KEY_LONG_PRESS_RELEASE : KEY_RELEASE;
                        debounce_state = STATE_WAIT_FOR_KEY_PRESS;
                        printf("STATE_WAIT_FOR_KEY_PRESS\n");
                    }
                }
                else
                {
                    debounce_state = STATE_WAIT_FOR_KEY_RELEASE;
                    printf("STATE_WAIT_FOR_KEY_RELEASE\n");
                }
                break;
        }

        // if (key_state)
        // {
        //     disable_systick();
        //     funDigitalWrite(blink_pin, FUN_LOW);
        // }

        if (key_event == KEY_LONG_PRESS_RELEASE)
        {
            key_event = KEY_NONE;
            funDigitalWrite(button_pin, FUN_LOW);  // Input pull-down
            // disable_systick();
            // funDigitalWrite(blink_pin, FUN_HIGH);
        }
        else if (key_event == KEY_RELEASE)
        {
            key_event   = KEY_NONE;
            blink_delay = (blink_delay == 500) ? 100 : 500;
            printf("Key Released. blink_delay = %ld\n", blink_delay);
            systick_init();  // Re-initiate systick, change LED blink interval.
        }

        Delay_Ms(DEBOUNCE_INTERVAL);
    }
}
