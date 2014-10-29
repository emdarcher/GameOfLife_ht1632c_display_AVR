//displays Conway's Game of Life on a ht1632c-based 32x8 LED Matrix
//and counts the generations on 3 multiplexed 7 segment displays
//This is controlled by an AVR ATtiny26 mcu
//code written by Ethan Durrant [emdarcher]

//some functions are based on code from this site:
// http://www.daqq.eu/?p=250
// which was an implementation of Conway's Game of Life
// on a 20x4 character LCD using an ATtiny2313 mcu.

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "ht1632c.h"
#include "seven_segs.h"

#define X_AXIS_LEN 32 //length of x axis
#define Y_AXIS_LEN 8 //length of y axis

#define BUTTON_BIT 6 //bit number on BUTTON_PORT to be used for button
#define BUTTON_DDR DDRB //DDRx for BUTTON_PORT
#define BUTTON_PORT PORTB //PORTx that the button is connected to
#define BUTTON_PIN PINB //PINx for the port the button is connected to

#define DO_YOU_WANT_TO_USE_WATCHDOG 1 //set if you want to use watchdog

#define DO_YOU_WANT_A_GENERATION_RESET_BUTTON 0 //do you want a generation reset button at all

#define DO_YOU_WANT_BUTTON_INT0 1  //set this if you want to use external
                                    //interrupt INT0 on PB6 for the button.

#define LOW_DIFF_THRESHOLD 42 //threshold of how many generations can pass
                                //with a low difference betweem each other
                                //before reset.
#define MED_DIFF_THRESHOLD 196 //same as above but for medium difference.

#define DO_YOU_WANT_ADC6_INPUT_TO_HT1632C_PWM 1 //set this to "1" if you
                                //want the ADC6 input used for adjusting
                                //the PWM/brightness setting of the ht1632c

uint8_t fb[X_AXIS_LEN];      /* framebuffer */
uint8_t state_storage[X_AXIS_LEN]; //area to store pixel states

volatile uint8_t update_gen_flag = 0;

//framebuffer functions
void clear_fb(void);
void push_fb(void);

//stuff for game of life things
static inline void get_new_states(void);
uint8_t get_new_pixel_state(uint8_t in_states[], int8_t x, int8_t y);
uint8_t get_current_pixel_state(uint8_t in[], int8_t x,int8_t y); 
uint8_t get_difference(uint8_t a[],uint8_t b[]);

//variables to store various difference counts
uint8_t low_diff_count=0;
uint8_t old_low_diff_count=0;
uint16_t med_diff_count=0;
uint16_t old_med_diff_count=0;

volatile uint16_t generation_count=0;

//#define INIT_BUTTON BUTTON_DDR &= ~(1<<BUTTON_BIT);BUTTON_PORT |= (1<<BUTTON_BIT);
static inline void init_button(void);

void init_srand(void);

//void init_timer0(void);
static inline void init_timer1(void);

static inline void init_ADC(void);

static inline void set_ht1632_bright_ADC(uint8_t adc_num);

void reset_grid(void);

//main code
int main(void)
{
//init stuff
    
    //init the ht1632c LED matrix driver chip
    ht1632c_init();
    
    //init the ADC
    init_ADC();
    
    //init srand() with a somewhat random number from ADC9's low bits
    init_srand();
    
    //init button stuff for input and pullup
    //and setup INT0 for button if you set DO_YOU_WANT_BUTTON_INT0
    init_button();
    
    //init timer1 for use in triggering an interrupt
    //on overflow, which updates the display and calculates new generation.
    init_timer1();
    
    //init the I/O for the 7 segment display control
    init_digit_pins();
    init_segment_pins();
    
    //reset the display with a "random" array using rand()
    reset_grid();
    
    //test glider
    //fb[29] = 0b00100000;
    //fb[30] = 0b00101000;
    //fb[31] = 0b00110000;
    
    //variable to store generation_count for display on 7 segment displays
    uint16_t g_count=0;
    
    #if DO_YOU_WANT_TO_USE_WATCHDOG==1
    //setup watchdog
    wdt_enable(WDTO_1S);
    #endif
    
    //enable global interrupts
    sei();
    
    //infinite loop
    while(1){
        
        //check if the update generation count flag has been set
        //by within the timer1 overflow interrupt.
        //if set put the updated value into g_count and reset the flag.
        if(update_gen_flag){
            g_count = generation_count;
            update_gen_flag=0;
        }
        write_number(g_count); //write the g_count to 7 segment displays
        //write_number(generation_count);
        
        //if the seven_seg_error flag is set (output is over 999 for 3 digits)
        //then reset the grid 
        if(seven_seg_error_flag){
            reset_grid();
        }
        
        #if DO_YOU_WANT_TO_USE_WATCHDOG==1
        //reset watchdog
        wdt_reset();
        #endif
        
    }
}
void clear_fb(void){
//clears the framebuffer
    uint8_t count;
    for(count=0;count<X_AXIS_LEN;count++){
        fb[count]=0;
    }
}

void push_fb(void){
//pushes frambuffer into the ht1632c chip in the display
    
    uint8_t i=X_AXIS_LEN;
    while(i--){
        ht1632c_data8((i*2),fb[i]);
    }
}

static inline void init_button(void){
    //setup for input
    BUTTON_DDR &= ~(1<<BUTTON_BIT);
    
    #if DO_YOU_WANT_A_GENERATION_RESET_BUTTON==1
    //enable pullup
    BUTTON_PORT |= (1<<BUTTON_BIT);
    
    #if DO_YOU_WANT_BUTTON_INT0
    //if you want the button to use INT0 for button on PB6
        
        //setup INT0 to trigger on falling edge
        MCUCR |= (1<<ISC01);
        //enable the INT0 external interrupt
        GIMSK |= (1<<INT0);
    #endif
    #endif
}

void init_srand(void){
    
    ADMUX |= 9;//set to ADC9 input
    
    //start adc
    ADCSR |= (1<<ADSC);
    loop_until_bit_is_clear(ADCSR, ADSC);//wait until done
    srand(ADCL); //for a pretty random adc reading
    
}

static inline void set_ht1632_bright_ADC(uint8_t adc_num){
    uint8_t temp_reg = ADMUX; //save current state
    
    ADMUX &= ~(0b11111); //clear bottom part
    ADMUX |= adc_num; //set to read the ADCx where x is adc_num
    
    //start conversion
    ADCSR |= (1<<ADSC);
    
    loop_until_bit_is_clear(ADCSR, ADSC);//wait until done
    
    ht1632c_bright(ADC/64);
    //generation_count=ADC/64;
    ADMUX = temp_reg; //reset ADMUX to original state
}

void reset_grid(void){
//resets the framebuffer with "random" values
    uint8_t k;
    for(k=0;k<X_AXIS_LEN;k++){
        fb[k] = ((uint8_t)rand() & 0xff);
    }
    generation_count=0;
}

uint8_t get_current_pixel_state(uint8_t in[], int8_t x,int8_t y){
//get the state (1==alive,0==dead), of a particular pixel/cell and return it

    //for wrapping the display axis so the 
    //Game of Life doesn't seem as restricted
    //this is called a toroidal array
    if(x < 0){ x = (X_AXIS_LEN - 1);}
    if(x == X_AXIS_LEN) {x = 0;}
    if(y < 0){ y = (Y_AXIS_LEN-1);}
    if(y == Y_AXIS_LEN) {y = 0;}
    
    //return the value
    return (in[x] & (1<<y));
}

uint8_t get_new_pixel_state(uint8_t in_states[], int8_t x,int8_t y){
    
    uint8_t n=0;//to store the neighbor count
    uint8_t state_out=0;
    
    //check on neighbors, see how many are alive.
    if(get_current_pixel_state(in_states, x-1,y)){n++;}
    if(get_current_pixel_state(in_states, x-1,y+1)){n++;}
    if(get_current_pixel_state(in_states, x-1,y-1)){n++;}
    
    if(get_current_pixel_state(in_states, x,y-1)){n++;}
    if(get_current_pixel_state(in_states, x,y+1)){n++;}
    
    if(get_current_pixel_state(in_states, x+1,y)){n++;}
    if(get_current_pixel_state(in_states, x+1,y+1)){n++;}
    if(get_current_pixel_state(in_states, x+1,y-1)){n++;}
    
    //now determine if dead or alive by neighbors,
    //these are implementing the rule's of Conway's Game of Life:
    /* from Wikipedia
     * Any live cell with fewer than two live neighbours dies, as if caused by under-population.
     * Any live cell with two or three live neighbours lives on to the next generation.
     * Any live cell with more than three live neighbours dies, as if by overcrowding.
     * Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
     */
    if((n<2)){state_out=0;}
    else if ((n<=3) && get_current_pixel_state(in_states, x, y)){state_out=1;}
    else if ((n==3)){state_out=1;}
    else if ((n>3)){state_out=0;}
    
    return state_out;
}

static inline void get_new_states(void){
//find all the new states and put them in the buffer
    
    //copy the current stuff into storage
    
    int8_t x=X_AXIS_LEN;
    while(x--){
        int8_t y=Y_AXIS_LEN;
        while(y--){
            if(get_new_pixel_state(fb, x, y)==1){
                state_storage[x] |= (1<<y);
            } else {
                state_storage[x] &= ~(1<<y);
            }
        }
    }
    //store the difference between the two generations in diff_val
    //to be used in finding when to reset.
    uint8_t diff_val= get_difference(state_storage,fb);
    
    if((diff_val <= 4)){
        //if diff_val is a low difference then increment it's counter
        low_diff_count++;
    }
    else if((diff_val<=8)){
        //if diff_val is a medium difference then increment that counter
        med_diff_count++;
    }
    else{
        //if neither, then decrement their counters to stay longer before reset
        if(low_diff_count > 0){
            low_diff_count--;
        }
        if(med_diff_count >0){
            med_diff_count--;
        }
    }
    #if DO_YOU_WANT_A_GENERATION_RESET_BUTTON==1
    #if DO_YOU_WANT_BUTTON_INT0==0
    //if you don't want to use INT0 for button
    //then this "if" statement will compile
    //which just checks the button pin's state whenever
    //this function runs
    if(bit_is_clear(BUTTON_PIN, BUTTON_BIT)){
        reset_grid();
    } 
    else 
    #endif
    #endif
    if(low_diff_count > LOW_DIFF_THRESHOLD){
    //if low_diff_count is above threshold, reset
        low_diff_count=0;
        reset_grid();
    }
    else if(med_diff_count > MED_DIFF_THRESHOLD){
    //if med_diff_count is above threshold, reset
        med_diff_count=0;
        reset_grid();
    }
    else{
    //if it is interesting enough so far then just add the new generation
    //to the framebuffer.
        for(x=0;x<X_AXIS_LEN;x++){
            //put the new values into the framebuffer
            fb[x] = state_storage[x];
        }
    }
}

uint8_t get_difference(uint8_t a[],uint8_t b[])
{//gets the amount of differences between two generations
    uint8_t x_v,y_v,diff=0;

    for(x_v=0; x_v < X_AXIS_LEN; x_v++)
    {
        for(y_v=0; y_v < Y_AXIS_LEN; y_v++)
        {
            //if changed from 0 to 1 or vise-versa, then increment the difference value
            if(( (get_current_pixel_state(a,x_v,y_v)==1) && (get_current_pixel_state(b,x_v,y_v) == 0)) 
            || ((get_current_pixel_state(a,x_v,y_v)==0) && (get_current_pixel_state(b,x_v,y_v)==1)))
            {
                diff++;
            }
        }
    }
    return diff;
}

/*
void init_timer0(void){
    
    //setup prescaler to CK/1024
    TCCR0 |= ( (1<<CS02) | (1<<CS00) );
    //enable timer0 overflow interrupt
    TIMSK |= (1<<TOIE0);
}*/

static inline void init_timer1(void){

    //set prescaler to CK/16384
    //with 8MHz clock, and 8bit timer/counter1
    //this prescaler should make it overflow every 0.52224 seconds
    //a good generation update rate for the Game of Life.
    TCCR1B |= ((1<<CS13)|(1<<CS12)|(1<<CS11)|(1<<CS10));
    //enable timer1 overflow interrupt
    TIMSK |= (1<<TOIE1);
}

static inline void init_ADC(void){
    //init the ADC
    
    DDRA &= ~(1<<7);//make sure it is set to input on PA7
    PORTA &= ~(1<<7);//make sure there are no pullups 
    //set clock prescaler to div 16
    ADCSR |= (1<<ADPS2);
    
    //enable the ADC
    ADCSR |= (1<<ADEN);
}

//----ISRs-----


ISR(TIMER1_OVF1_vect){
    //timer1 overflow interrupt service routine
        
        //increment the generation count
        generation_count++;
        //push framebuffer to the display
        push_fb();
        //get the new states and add them to the framebuffer,
        //or reset the display if there isn't enough action
        get_new_states();
        
        //set the update_gen_flag that will alert the "if" statement
        //in the main while(1) loop, so it will update the 7 segment display
        //with the new generation count
        update_gen_flag=1;
        
        
        #if DO_YOU_WANT_ADC6_INPUT_TO_HT1632C_PWM
        //if you set the DO_YOU_WANT_ADC6_INPUT_TO_HT1632C_PWM to "1"
        //then this below code will compile
        
        //adc stuff to control pwm
        set_ht1632_bright_ADC(6);
        
        #endif //end of this little snippet
}

#if DO_YOU_WANT_A_GENERATION_RESET_BUTTON==1
#if DO_YOU_WANT_BUTTON_INT0
//if you want a button to use INT0 for button on PB6

ISR(INT0_vect){
//INT0 ISR, activated by falling edge
//made when button pressed

    //reset and "randomize"
    reset_grid();
}

#endif

#endif
