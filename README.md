GameOfLife_ht1632c_display_AVR
===============================


Code for to display Conway's Game of Life on a ht1632c-based 32x8 LED matrix board, controlled by an AVR ATtiny26 microcontroller. The extra I/O pins on the ATtiny26 are used to control 3 multiplexed seven-segment displays to show the generation count. The code has counters of generations that have low differences, and when it reaches a threshold it resets the display with a new "random" pattern. This is to keep it interesting, and to prevent oscillators from staying indefinitely and preventing reset. 

ADDITIONAL FEATURES:
---------------------

    * There is a button connected to PB6 of the ATtiny26, which triggers external interrupt INT0 which can reset the display if the spectator desires to do so. Using this button with INT0 is optional, and can be disabled by clearing `DO_YOU_WANT_BUTTON_INT0` to `0` in `main.c` before compiling.

    * There is the option to have a potentiometer or other analog sensor (photoresistor/LDR perhaps?) connected to PB7 (ADC6) to control the PWM brightness setting of the ht1632c-based display! This is also optional, and can be disabled by clearing `DO_YOU_WANT_ADC6_INPUT_TO_HT1632C_PWM` to `0` in `main.c` before compiling.

Original development and testing of the code had been done within my [driving_ht1632c_AVR](https://github.com/emdarcher/driving_ht1632c_AVR) repository.
