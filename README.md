# CH32V003 Soft Latching Power Circuits

The Soft Latching Power Circuits is suitable for battery powered applications, there is no power consumption after powered off.

## Schematic

- `R1` and `C1` keep `Q1` off, `R2` keeps `Q2` off.
- When button `SW1` is pushed, the gate of `Q1` is pulled low and power on MCU. The MCU immediately pull `PA1` high to latch Q2 and Q1.
- To power off the circuit, pull `PA1` low to turn off `Q1` and `Q2`.

![Alt text](schematic/CH32V003_Soft_Latching/CH32V003_Soft_Latching.png)

## Notes

The circuit works perfectly using CH32V003!

However, the first time I tried the circuit was using a CH552, it never works stably, it is probably because the CH552 GPIOs have internal pull-up resistor enabled in default state (Quasi-bidirectional mode (standard 8051), open-drain output, support input, pin has pull-up resistor), thus it may not fully turn `Q2` off. On the contrary, the CH32V003 GPIOs are in high impedance mode by default.

## References

- [Andrew Levido: Soft Latching Power Circuits](https://circuitcellar.com/resources/quickbits/soft-latching-power-circuits/)
- [CNLohr: ch32v003fun](https://github.com/cnlohr/ch32v003fun)
