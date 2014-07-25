Code for to display Conway's Game of Life on a ht1632c-based 32x8 LED matrix board, controlled by an AVR ATtiny26 microcontroller. The extra I/O pins on the ATtiny26 are used to control 3 multiplexed seven-segment displays to show the generation count. The code has counters of generations that have low differences, and when it reaches a threshold it resets the display with a new "random" pattern. This is to keep it interesting, and to prevent oscillators from staying indefinitely and preventing reset. In addition, there is a button connected to PB6 of the ATtiny26, which triggers external interrupt INT0 which can reset the display if the spectator desires to do so.

####UPDATE:
    Now there is the option to have a potentiometer or other analog sensor to PB7 (ADC6) and control the PWM brightmess of the dispay!

Development and testing of the code has been done within my [driving_ht1632c_AVR](https://github.com/emdarcher/driving_ht1632c_AVR) repository.
