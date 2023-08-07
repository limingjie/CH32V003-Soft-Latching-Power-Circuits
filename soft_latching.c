/*
 * CH32V003J4M6 (SOP-8) Soft Latching
 *
 * Reference:
 *  - https://github.com/cnlohr/ch32v003fun
 *  - https://circuitcellar.com/resources/quickbits/soft-latching-power-circuits/
 *
 * Aug 2023 by Li Mingjie
 *  - Email:  limingjie@outlook.com
 *  - GitHub: https://github.com/limingjie/
 */

#include "ch32v003fun.h"
#include "ch32v003_GPIO_branchless.h"

int main()
{
    SystemInit();
    Delay_Ms(100);

    // Power on latch
    GPIO_port_enable(GPIO_port_A);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 1), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 1), high);

    // Turn LED on
    GPIO_port_enable(GPIO_port_C);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_C, 1), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);

    while (1)
    {
        // Blink the LED
        uint32_t delay = 500;
        for (uint8_t i = 0; i < 10; i++)
        {
            if (i == 5)
                delay = 100;

            GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_C, 1), low);
            Delay_Ms(delay);
            GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_C, 1), high);
            Delay_Ms(delay);
        }

        // Power off
        GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 1), low);
    }
}
