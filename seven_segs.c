//my :small little library for driving multiplexed seven segment displays

//the functions

#include "seven_segs.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#if USE_DIG_BIT_ARRAY==1
uint8_t digit_bits[] PROGMEM = { DIG_0, DIG_1, DIG_2 };
#endif
//const uint8_t  num_digits = sizeof(digit_bits)/2;
const uint8_t num_digits = 3;
uint8_t seven_seg_error_flag=0;

//const 
uint8_t number_seg_bytes[]  PROGMEM = {
//       unconfigured
//ABCDEFG^
0b11111101,//0
0b01100000,//1
0b11011011,//2
0b11110011,//3
0b01100110,//4
0b10110111,//5
0b10111111,//6
0b11100001,//7
0b11111111,//8
0b11100111,//9
0b10011111, //'E' for error
};

void init_digit_pins(void){
    
    //setup bits 0-2 in DDRB for output for digits 0-2
    //DDRB |= ALL_DIGS;
    DIGIT_DDR |= ALL_DIGS;
}

void init_segment_pins(void){
    //setup all segs as output
    SEGMENT_DDR |= ALL_SEGS;
}


void write_digit(int8_t num, uint8_t dig){
    uint8_t k;
    uint8_t out_byte;
    
    if((num < 10) && (num >= 0)){
    //out_byte = number_seg_bytes[num];
    out_byte = pgm_read_byte(&number_seg_bytes[num]);
    } else {
    
    //out_byte = number_seg_bytes[10];
    out_byte = pgm_read_byte(&number_seg_bytes[10]);
    }
    
    //output the byte to the port, shift right 1 bit to correctly
    //use the values from number_seg_bytes.
    //write_segs((out_byte>>1));
    SEGMENT_PORT = (out_byte>>1);
    
    for( k = 0; k < num_digits; k++){
        if ( k == dig ){
                //PORTB |= pgm_read_byte(&digit_bits[k]);
                #if USE_DIG_BIT_ARRAY==1
                DIGIT_PORT |= pgm_read_byte(&digit_bits[k]);
                #else
                DIGIT_PORT |= (DIG_0 >> k); 
                #endif
        } else {
                //PORTB &= ~(pgm_read_byte(&digit_bits[k]));
                #if USE_DIG_BIT_ARRAY==1
                DIGIT_PORT &= ~(pgm_read_byte(&digit_bits[k]));
                #else
                DIGIT_PORT &= ~(DIG_0 >> k);
                #endif
        }
    }
    _delay_ms(DIGIT_DELAY_MS);
    
}

void msg_error(void){
    write_digit(10, 0);
    seven_seg_error_flag=1;
}

void write_number(int16_t number){
        uint8_t h;
        int16_t format_num = number;
        #if 1//check if number is too big ot not
        //if (number > 999){ format_num = 999;}
        if (!((number < 1000) && (number >= 0))){
            format_num = 999;
        }
            //formats number based on digits to correct digits on display
            for(h=0;h < num_digits;h++){
                write_digit(format_num % 10, h);
                format_num /= 10;
            }         
        //} 
        //else {
           // msg_error();
        //}
        #endif
    #if 0
    uint8_t d_nums[3];
    d_nums[2] = number / 100;
    number -= d_nums[2] * 100;
    d_nums[1] = number / 10;
    number -= d_nums[1] * 10;
    d_nums[0] = number;
    for(h=0;h<num_digits;h++){
        write_digit(d_nums[h],h);
    }
    #endif
    #if 0
    uint8_t msd,nsd,lsd;
    msd = number / 100;
    number -= msd * 100;
    write_digit(msd,2);
    nsd = number / 10;
    number -= nsd * 10;
    write_digit(nsd,1);
    lsd = number;
    write_digit(lsd,0);
    /*for(h=0;h<num_digits;h++){
        write_digit(d_nums[h],h);
    }*/
    #endif

}


//this is so we avoid writing to PORTA bit 7, which is connected to the
//ADC for brightness control.
void write_segs(uint8_t byte){
    uint8_t o=7;
    while(o--){
        if(byte & (1<<o)){
            SEGMENT_PORT |= (1<<o);
        }
        else{
            SEGMENT_PORT &= ~(1<<o);
        }
    }
}
