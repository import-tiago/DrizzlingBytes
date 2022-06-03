#include <msp430.h>

#define FAST 100000
#define SLOW 1000000
#define LED BIT7

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    P2OUT &= ~LED;
    P2DIR |= LED;

    PM5CTL0 &= ~LOCKLPM5;

    while (1) {
        P2OUT ^= LED;
        __delay_cycles(SLOW);
    }
}
