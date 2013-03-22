/* Filename: BitterPattern.c
 * Author:   A. Wilson
 * Date:     March 2013
 *
 * Description & use:
 *   This code was developed for use in conjunction with IOIO firmware provided
 * by Ytai Ben-Tsvi. It allows direct pin manipulation of the IOIO board output
 * I/O using user-definable bit-patterns and frequencies. There are two functions
 * that can be called directly by the user:
 *
 * bitPattern() - Outputs a pattern of 0's and 1's according to the parameters
 * repeatPattern() - Makes multiple calls to bitPattern at a fixed interval
 *
 * The relevant parameters are:
 *   bitPattern[] - Arbitrary length string of 0's and 1's
 *   inversion - Polarity indicator (0 to use bitPattern as-is, 1 to invert)
 *   frequency - 1 Hz to 30 kHz, frequency of the bit change
 *   times - number of times to repeat pattern
 *   tdelay - time (in ms) to delay between repeated pattern
 *
 */

#include "Compiler.h"
#include "timer.h"
#include "platform.h"

/*************************************************************/
/* GENERIC INCLUDES THAT ARE PROBABLY IN OTHER FILES ALREADY */
/*************************************************************/
#define GetSystemClock()            32000000UL
#define GetPeripheralClock()        (GetSystemClock())
#define GetInstructionClock()       (GetSystemClock() / 2)

/***********************************/
/* DEFINE MESSAGES FOR HEAT ON/OFF */
/***********************************/
//"HEAT_3RD_ON" uses RF_68_to80_ON command
#define HEAT_3RD_ON  "101010101010101010101001001101100101011101001101001001011110001100011001010101011001101100101001011001011000110001110101011110001"
//"HEAT_3RD_OFF" uses RF_67_to60_OFF command
#define HEAT_3RD_OFF "101010101010101010101001001101100101011101001101001000111110001100011000111011001001101100101001011001011000110001110111001100101"

/************************************/
/* DEFINE HARDWARE SPECIFIC PINOUTS */
/************************************/
#if HARDWARE >= HARDWARE_IOIO0000 && HARDWARE <= HARDWARE_IOIO0003
#define RFctrl_init() do { _ODF0 = 0; _LATF0 = 1; _TRISF0 = 0; } while (0)
#define RFctrl_release() do { _ODF0 = 0; _LATF0 = 0; _TRISF0 = 1; } while (0)
#define RFctrl_force_hi() do { _ODF0 = 0; _LATF0 = 1; _TRISF0 = 0; } while (0)
#define RFctrl_force_lo() do { _ODF0 = 0; _LATF0 = 0; _TRISF0 = 0; } while (0)

#define RFtx_init() do { _ODF1 = 0; _LATF1 = 0; _TRISF1 = 0; } while (0)
#define RFtx_on()   do { _LATF1 = 0; } while (0)
#define RFtx_off()  do { _LATF1 = 1; } while (0)
#define RFtx_release() do { _ODF1 = 1; _LATF1 = 1; _TRISF1 = 1; } while (0)

#else
  #error Unknown hardware
#endif

/*********************************************/
/* DEFINE ASSEMBLY SPECIFIC CLOCK MULTIPLIER */
/* (for inaccurate clock timing)             */
/*********************************************/
#define CLK_SCALE 1.05

/*************************************/
/* MAIN CODE FOR TESTING DEVELOPMENT */
/* (comment out before integrating)  */
/*************************************/
/*int main() {
  led_init();
  while (1) {
      repeatPattern(HEAT_3RD_ON, sizeof(HEAT_3RD_ON)-1, 0, 1000, 8, 1000);
      DelayMs(5000*1.05);
      repeatPattern(HEAT_3RD_OFF, sizeof(HEAT_3RD_OFF)-1, 0, 1000, 8, 1000);
      DelayMs(60000);
  }
  return 0;
}
*/

int repeatPattern(char pattern[], int inversion, int freq, int times, int tdelay) {
    // This function calls bitPattern repeatedly for a fixed number of times with a fixed delay
    int repeat;
    for (repeat = 0; repeat < times; repeat++) {
        bitPattern(pattern, inversion, freq);
        if (repeat < times - 1) { DelayMs(tdelay*CLK_SCALE);}
    }
}

int bitPattern(char pattern[], int inversion, int freq) {
    int i, bitFreq, bitLevel;
    int patternLen = strlen(pattern);

    RFctrl_init();
    RFtx_init();
    led_on();

    Delay10us(10);   // Wait at least 15 us for RF chip to turn on (2x margin)

    for (i = 0; i < patternLen; i++) {
        bitLevel = 1 * ((pattern[i] == '1') ^ (inversion == 1));
        sendBit(freq, bitLevel);
    }

    RFtx_release();
    DelayMs(10);   // Wait 10ms before turning off RF chip
    RFctrl_release();

    led_off();

  return 0;
}

int sendBit(int bitFreq, int bitLevel) {
    if (bitLevel == 1) {
        RFtx_on();
        //led_on();
    }
    else {
        RFtx_off();
        //led_off();
    }
    Delay10us(100000*CLK_SCALE/bitFreq);

    return 0;
}


/*************************************/
/* ONLY TEST FUCTIONS BELOW THIS LINE*/
/*************************************/
int Blink(int freq) {
    DelayMs(1000/freq/2);
    led_on();
    DelayMs(1000/freq/2);
    led_off();
    return 0;
}

int tuningBits(int fudge) {
    //TUNING CIRCUIT - OUTPUTS 100 pulses, total width should be 100 ms
    int i;
    for (i = 0; i < 100; i++) {
        RFtx_off();
        Delay10us((100000 + fudge)/1000/2);
        RFtx_on();
        Delay10us((100000 + fudge)/1000/2);
    }

}

int fixedFrequency(int freq) {
    // NOTE: Max frequency of ~30kHz (outputs 50kHz)
    RFctrl_init();
    RFtx_init();
    led_on();
    while (1) {
        RFctrl_force_hi();
        Delay10us((100000*CLK_SCALE) / freq / 2);
        RFctrl_force_lo();
        Delay10us((100000*CLK_SCALE) / freq / 2);
    }
    led_off();
}

