#include <msp430.h> 

#define FAST 100000
#define SLOW 1000000
#define LED BIT0

void main(void) {

    WDTCTL = WDTPW | WDTHOLD;

    P5OUT &= ~LED;
    P5DIR |= LED;

    PM5CTL0 &= ~LOCKLPM5;

    while (1) {
        P5OUT ^= LED;
        __delay_cycles(SLOW);
    }
}
