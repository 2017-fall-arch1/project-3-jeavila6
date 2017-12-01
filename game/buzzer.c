#include <msp430.h>
#include "buzzer.h"
#include "libTimer.h"

#define MIN_PERIOD 1000
#define MAX_PERIOD 4000

static unsigned int period = 1000;
static signed int rate = 200;
static int count = 0;

void buzzer_init() {
    timerAUpmode(); // Used to drive speaker
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL  &= ~BIT7; 
    P2SEL  |= BIT6;
    P2DIR   = BIT6; // Enable output to speaker (P2.6)
}

void buzzerSetPeriod(short cycles) {
    
        CCR0 = cycles; 
        
        CCR1 = cycles >> 1; // One half cycle
    
}
