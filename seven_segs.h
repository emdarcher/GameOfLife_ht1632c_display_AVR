//my small little library for driving multiplexed seven segment displays


//header file with seven segment stuff

#ifndef SEVEN_SEGS
#define SEVEN_SEGS

#include <stdint.h>
#include <avr/eeprom.h>
/*
segments
 ---A---
|       |
F       B
|___G___|
|       |
E       C
|___D___| O dp
*/

/*
 * pinout of an individual display
 *      |------|
 * GND--|  __  |--  A
 * F  --| |  | |--  B
 * G  --| |--| |--  C
 * E  --| |__| |-- dp
 * D  --|_____O|--GND
 * 
 * 
 * the display is common cathode
 */
 
 /*
  * The digit pins will be connected to NPN transistors
  * that will sink the corresponding digit's cathodes
  * 
  */

#define SEGMENT_DDR DDRA
#define SEGMENT_PORT PORTA

#define DIGIT_DDR DDRB
#define DIGIT_PORT PORTB

//these are the bits of a AVR port
//that go to particular segments
#define SEG_A (1<<6)
#define SEG_B (1<<5)
#define SEG_C (1<<4)
#define SEG_D (1<<3)
#define SEG_E (1<<2)
#define SEG_F (1<<1)
#define SEG_G (1<<0)

#define ALL_SEGS ( SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G )

//make sure to handle digit selection in software
//set these bits to the bits of the PORT that the digit control will be on
#define DIG_0 (1<<2)
#define DIG_1 (1<<1)
#define DIG_2 (1<<0)

//remember to add any newly defines digits here
#define ALL_DIGS ( DIG_0 | DIG_1 | DIG_2 )

#define INIT_SEGMENT_PINS SEGMENT_DDR |= ALL_SEGS

#define DIGIT_DELAY_MS 1 //ms to wait before switching to next digit

//extern const uint8_t digit_bits[];

extern const uint8_t  num_digits;

extern uint8_t seven_seg_error_flag;

void init_digit_pins(void);
void init_segment_pins(void);

void msg_error(void);

void write_number(int16_t number);
void write_digit(int8_t num, uint8_t dig);

void write_segs(uint8_t byte);


#endif
