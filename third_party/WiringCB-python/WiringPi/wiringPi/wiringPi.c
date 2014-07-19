/*
 * wiringPi:
 *	Arduino compatable (ish) Wiring library for the Raspberry Pi
 *	Copyright (c) 2012 Gordon Henderson
 *	Additional code for pwmSetClock by Chris Hall <chris@kchall.plus.com>
 *
 *	Thanks to code samples from Gert Jan van Loo and the
 *	BCM2835 ARM Peripherals manual, however it's missing
 *	the clock section /grr/mutter/
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

// Revisions:
//	19 Jul 2012:
//		Moved to the LGPL
//		Added an abstraction layer to the main routines to save a tiny
//		bit of run-time and make the clode a little cleaner (if a little
//		larger)
//		Added waitForInterrupt code
//		Added piHiPri code
//
//	 9 Jul 2012:
//		Added in support to use the /sys/class/gpio interface.
//	 2 Jul 2012:
//		Fixed a few more bugs to do with range-checking when in GPIO mode.
//	11 Jun 2012:
//		Fixed some typos.
//		Added c++ support for the .h file
//		Added a new function to allow for using my "pin" numbers, or native
//			GPIO pin numbers.

//		Removed my busy-loop delay and replaced it with a call to delayMicroseconds
//
//	02 May 2012:
//		Added in the 2 UART pins
//		Change maxPins to numPins to more accurately reflect purpose


#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include "wiringPi.h"

#define CB_BOARD


#ifndef	TRUE
#define	TRUE	(1==1)
#define	FALSE	(1==2)
#endif
/* for mmap cubieboard */
#define SUNXI_GPIO_BASE (0x01C20800)
#define MAP_SIZE	(4096*2)
#define MAP_MASK	(MAP_SIZE - 1)
//sunxi_pwm
#define SUNXI_PWM_BASE (0x01c20e00)
#define SUNXI_PWM_CTRL_REG  (SUNXI_PWM_BASE)
#define SUNXI_PWM_CH0_PERIOD  (SUNXI_PWM_BASE + 0x4)
#define SUNXI_PWM_CH1_PERIOD  (SUNXI_PWM_BASE + 0x8)

#define SUNXI_PWM_CH0_EN			(1 << 4)
#define SUNXI_PWM_CH0_ACT_STA		(1 << 5)
#define SUNXI_PWM_SCLK_CH0_GATING	(1 << 6)
#define SUNXI_PWM_CH0_MS_MODE		(1 << 7) //pulse mode
#define SUNXI_PWM_CH0_PUL_START		(1 << 8)

#define SUNXI_PWM_CH1_EN			(1 << 19)
#define SUNXI_PWM_CH1_ACT_STA		(1 << 20)
#define SUNXI_PWM_SCLK_CH1_GATING	(1 << 21)
#define SUNXI_PWM_CH1_MS_MODE		(1 << 22) //pulse mode
#define SUNXI_PWM_CH1_PUL_START		(1 << 23)


#define PWM_CLK_DIV_120 	0
#define PWM_CLK_DIV_180		1
#define PWM_CLK_DIV_240		2
#define PWM_CLK_DIV_360		3
#define PWM_CLK_DIV_480		4
#define PWM_CLK_DIV_12K		8
#define PWM_CLK_DIV_24K		9
#define PWM_CLK_DIV_36K		10
#define PWM_CLK_DIV_48K		11
#define PWM_CLK_DIV_72K		12

// Environment Variables

#define	ENV_DEBUG	"WIRINGPI_DEBUG"
#define	ENV_CODES	"WIRINGPI_CODES"


// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices
#ifdef CB_BOARD
#define	PI_GPIO_MASK	(0xFFFFFFC0)
//#define	MAX_PIN_NUM		(0x120)  //287
#define	MAX_PIN_NUM		(0x61)  //97
#else
#define	PI_GPIO_MASK	(0xFFFFFFC0)
#define	MAX_PIN_NUM		(0x40)  //64
#endif

static struct wiringPiNodeStruct *wiringPiNodes = NULL ;

// BCM Magic

#define	BCM_PASSWORD		0x5A000000


// The BCM2835 has 54 GPIO pins.
//	BCM2835 data sheet, Page 90 onwards.
//	There are 6 control registers, each control the functions of a block
//	of 10 pins.
//	Each control register has 10 sets of 3 bits per GPIO pin - the ALT values
//
//	000 = GPIO Pin X is an input
//	001 = GPIO Pin X is an output
//	100 = GPIO Pin X takes alternate function 0
//	101 = GPIO Pin X takes alternate function 1
//	110 = GPIO Pin X takes alternate function 2
//	111 = GPIO Pin X takes alternate function 3
//	011 = GPIO Pin X takes alternate function 4
//	010 = GPIO Pin X takes alternate function 5
//
// So the 3 bits for port X are:
//	X / 10 + ((X % 10) * 3)

// Port function select bits

#define	FSEL_INPT		0b000
#define	FSEL_OUTP		0b001
#define	FSEL_ALT0		0b100
#define	FSEL_ALT0		0b100
#define	FSEL_ALT1		0b101
#define	FSEL_ALT2		0b110
#define	FSEL_ALT3		0b111
#define	FSEL_ALT4		0b011
#define	FSEL_ALT5		0b010

// Access from ARM Running Linux
//	Taken from Gert/Doms code. Some of this is not in the manual
//	that I can find )-:
#ifdef CB_BOARD
	#define GPIO_PADS		(0x00100000)
	#define CLOCK_BASE		(0x00101000)
//	addr should 4K*n
//	#define GPIO_BASE		(SUNXI_GPIO_BASE)
	#define GPIO_BASE		(0x01C20000)
	#define GPIO_TIMER		(0x0000B000)
	#define GPIO_PWM		(0x01c20000)  //need 4k*n
#else
	#define BCM2708_PERI_BASE	                     0x20000000
	#define GPIO_PADS		(BCM2708_PERI_BASE + 0x00100000)
	#define CLOCK_BASE		(BCM2708_PERI_BASE + 0x00101000)
	#define GPIO_BASE		(BCM2708_PERI_BASE + 0x00200000)
	#define GPIO_TIMER		(BCM2708_PERI_BASE + 0x0000B000)
	#define GPIO_PWM		(BCM2708_PERI_BASE + 0x0020C000)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

// PWM
//	Word offsets into the PWM control region
#define	PWM_CONTROL 0
#define	PWM_STATUS  1
#define	PWM0_RANGE  4
#define	PWM0_DATA   5
#define	PWM1_RANGE  8
#define	PWM1_DATA   9

//	Clock regsiter offsets

#define	PWMCLK_CNTL	40
#define	PWMCLK_DIV	41

#define	PWM0_MS_MODE    0x0080  // Run in MS mode
#define	PWM0_USEFIFO    0x0020  // Data from FIFO
#define	PWM0_REVPOLAR   0x0010  // Reverse polarity
#define	PWM0_OFFSTATE   0x0008  // Ouput Off state
#define	PWM0_REPEATFF   0x0004  // Repeat last value if FIFO empty
#define	PWM0_SERIAL     0x0002  // Run in serial mode
#define	PWM0_ENABLE     0x0001  // Channel Enable

#define	PWM1_MS_MODE    0x8000  // Run in MS mode
#define	PWM1_USEFIFO    0x2000  // Data from FIFO
#define	PWM1_REVPOLAR   0x1000  // Reverse polarity
#define	PWM1_OFFSTATE   0x0800  // Ouput Off state
#define	PWM1_REPEATFF   0x0400  // Repeat last value if FIFO empty
#define	PWM1_SERIAL     0x0200  // Run in serial mode
#define	PWM1_ENABLE     0x0100  // Channel Enable

// Timer
//	Word offsets

#define	TIMER_LOAD	(0x400 >> 2)
#define	TIMER_VALUE	(0x404 >> 2)
#define	TIMER_CONTROL	(0x408 >> 2)
#define	TIMER_IRQ_CLR	(0x40C >> 2)
#define	TIMER_IRQ_RAW	(0x410 >> 2)
#define	TIMER_IRQ_MASK	(0x414 >> 2)
#define	TIMER_RELOAD	(0x418 >> 2)
#define	TIMER_PRE_DIV	(0x41C >> 2)
#define	TIMER_COUNTER	(0x420 >> 2)

// Locals to hold pointers to the hardware

static volatile uint32_t *gpio ;
static volatile uint32_t *pwm ;
static volatile uint32_t *clk ;
static volatile uint32_t *pads ;

#ifdef	USE_TIMER
static volatile uint32_t *timer ;
static volatile uint32_t *timerIrqRaw ;
#endif

// Time for easy calculations

static uint64_t epochMilli, epochMicro ;

// Misc

static int wiringPiMode = WPI_MODE_UNINITIALISED ;
static int wiringPinMode = WPI_MODE_UNINITIALISED ;

// Debugging & Return codes

int wiringPiDebug = FALSE ;
int wiringPiCodes = FALSE ;

// sysFds:
//	Map a file descriptor from the /sys/class/gpio/gpioX/value

static int sysFds [64] ;

// ISR Data

static void (*isrFunctions [64])(void) ;

//gootoomoon sunxi func
void sunxi_pwm_set_clk(int clk);
void sunxi_pwm_set_enable(int en);
void sunxi_pwm_set_mode(int mode);
uint32_t sunxi_pwm_get_period(void);
void sunxi_pwm_set_period(int period_cys);
void sunxi_pwm_set_act(int act_cys);

int wiringPiSetupPhys (void);
void pullUpDnControl (int pin, int pud);
// Doing it the Arduino way with lookup tables...
//	Yes, it's probably more innefficient than all the bit-twidling, but it
//	does tend to make it all a bit clearer. At least to me!

// pinToGpio:
//	Take a Wiring pin (0 through X) and re-map it to the BCM_GPIO pin
//	Cope for 2 different board revisions here.

static int *pinToGpio ;

static int pinToGpioR1 [64] =   //gootoomoon  this may up to 96 pin [128?]
{
  17, 18, 21, 22, 23, 24, 25, 4,	// From the Original Wiki - GPIO 0 through 7:	wpi  0 -  7
   0,  1,				// I2C  - SDA0, SCL0				wpi  8 -  9
   8,  7,				// SPI  - CE1, CE0				wpi 10 - 11
  10,  9, 11, 				// SPI  - MOSI, MISO, SCLK			wpi 12 - 14
  14, 15,				// UART - Tx, Rx				wpi 15 - 16

// Padding:

      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 31
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 47
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 63
} ;

static int pinToGpioR2 [64] =
{
  17, 18, 27, 22, 23, 24, 25, 4,	// From the Original Wiki - GPIO 0 through 7:	wpi  0 -  7
   2,  3,				// I2C  - SDA0, SCL0				wpi  8 -  9
   8,  7,				// SPI  - CE1, CE0				wpi 10 - 11
  10,  9, 11, 				// SPI  - MOSI, MISO, SCLK			wpi 12 - 14
  14, 15,				// UART - Tx, Rx				wpi 15 - 16
  28, 29, 30, 31,			// New GPIOs 8 though 11			wpi 17 - 20

// Padding:

                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 31
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 47
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 63
} ;

// physToGpio:
//	Take a physical pin (1 through 26) and re-map it to the BCM_GPIO pin
//	Cope for 2 different board revisions here.

static int *physToGpio ;

static int physToGpioR1 [64] =
{
  -1,		// 0
  -1, -1,	// 1, 2
   0, -1,
   1, -1,
   4, 14,
  -1, 15,
  17, 18,
  21, -1,
  22, 23,
  -1, 24,
  10, -1,
   9, 25,
  11,  8,
  -1,  7,	// 25, 26

// Padding:

                                              -1, -1, -1, -1, -1,	// ... 31
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 47
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 63
} ;

static int physToGpioR2 [64] =
{
  -1,		// 0
  -1, -1,	// 1, 2
   2, -1,
   3, -1,
   4, 14,
  -1, 15,
  17, 18,
  27, -1,
  22, 23,
  -1, 24,
  10, -1,
   9, 25,
  11,  8,
  -1,  7,	// 25, 26

// Padding:

                                              -1, -1, -1, -1, -1,	// ... 31
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 47
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	// ... 63
} ;

// gpioToGPFSEL:
//	Map a BCM_GPIO pin to it's Function Selection
//	control port. (GPFSEL 0-5)
//	Groups of 10 - 3 bits per Function - 30 bits per port

static uint8_t gpioToGPFSEL [] =
{
  0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,
} ;


// gpioToShift
//	Define the shift up for the 3 bits per pin in each GPFSEL port

static uint8_t gpioToShift [] =
{
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
} ;

// gpioToGPSET:
//	(Word) offset to the GPIO Set registers for each GPIO pin

static uint8_t gpioToGPSET [] =
{
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
} ;

// gpioToGPCLR:
//	(Word) offset to the GPIO Clear registers for each GPIO pin

static uint8_t gpioToGPCLR [] =
{
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
  11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
} ;


// gpioToGPLEV:
//	(Word) offset to the GPIO Input level registers for each GPIO pin

static uint8_t gpioToGPLEV [] =
{
  13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
  14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
} ;


#ifdef notYetReady
// gpioToEDS
//	(Word) offset to the Event Detect Status

static uint8_t gpioToEDS [] =
{
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
} ;

// gpioToREN
//	(Word) offset to the Rising edgde ENable register

static uint8_t gpioToREN [] =
{
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
} ;

// gpioToFEN
//	(Word) offset to the Falling edgde ENable register

static uint8_t gpioToFEN [] =
{
  22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
  23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
} ;
#endif


// GPPUD:
//	GPIO Pin pull up/down register

#define	GPPUD	37

// gpioToPUDCLK
//	(Word) offset to the Pull Up Down Clock regsiter

static uint8_t gpioToPUDCLK [] =
{
  38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,
  39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,
} ;


// gpioToPwmALT
//	the ALT value to put a GPIO pin into PWM mode

static uint8_t gpioToPwmALT [] =
{
          0,         0,         0,         0,         0,         0,         0,         0,	//  0 ->  7
          0,         0,         0,         0, FSEL_ALT0, FSEL_ALT0,         0,         0, 	//  8 -> 15
          0,         0, FSEL_ALT5, FSEL_ALT5,         0,         0,         0,         0, 	// 16 -> 23
          0,         0,         0,         0,         0,         0,         0,         0,	// 24 -> 31
          0,         0,         0,         0,         0,         0,         0,         0,	// 32 -> 39
  FSEL_ALT0, FSEL_ALT0,         0,         0,         0, FSEL_ALT0,         0,         0,	// 40 -> 47
          0,         0,         0,         0,         0,         0,         0,         0,	// 48 -> 55
          0,         0,         0,         0,         0,         0,         0,         0,	// 56 -> 63
} ;


// gpioToPwmPort
//	The port value to put a GPIO pin into PWM mode

static uint8_t gpioToPwmPort [] =
{
          0,         0,         0,         0,         0,         0,         0,         0,	//  0 ->  7
          0,         0,         0,         0, PWM0_DATA, PWM1_DATA,         0,         0, 	//  8 -> 15
          0,         0, PWM0_DATA, PWM1_DATA,         0,         0,         0,         0, 	// 16 -> 23
          0,         0,         0,         0,         0,         0,         0,         0,	// 24 -> 31
          0,         0,         0,         0,         0,         0,         0,         0,	// 32 -> 39
  PWM0_DATA, PWM1_DATA,         0,         0,         0, PWM1_DATA,         0,         0,	// 40 -> 47
          0,         0,         0,         0,         0,         0,         0,         0,	// 48 -> 55
          0,         0,         0,         0,         0,         0,         0,         0,	// 56 -> 63

} ;

// gpioToGpClkALT:
//	ALT value to put a GPIO pin into GP Clock mode.
//	On the Pi we can really only use BCM_GPIO_4 and BCM_GPIO_21
//	for clocks 0 and 1 respectively, however I'll include the full
//	list for completeness - maybe one day...

#define	GPIO_CLOCK_SOURCE	1

// gpioToGpClkALT0:

static uint8_t gpioToGpClkALT0 [] =
{
          0,         0,         0,         0, FSEL_ALT0, FSEL_ALT0, FSEL_ALT0,         0,	//  0 ->  7
          0,         0,         0,         0,         0,         0,         0,         0, 	//  8 -> 15
          0,         0,         0,         0, FSEL_ALT5, FSEL_ALT5,         0,         0, 	// 16 -> 23
          0,         0,         0,         0,         0,         0,         0,         0,	// 24 -> 31
  FSEL_ALT0,         0, FSEL_ALT0,         0,         0,         0,         0,         0,	// 32 -> 39
          0,         0, FSEL_ALT0, FSEL_ALT0, FSEL_ALT0,         0,         0,         0,	// 40 -> 47
          0,         0,         0,         0,         0,         0,         0,         0,	// 48 -> 55
          0,         0,         0,         0,         0,         0,         0,         0,	// 56 -> 63
} ;

// gpioToClk:
//	(word) Offsets to the clock Control and Divisor register

static uint8_t gpioToClkCon [] =
{
         -1,        -1,        -1,        -1,        28,        30,        32,        -1,	//  0 ->  7
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1, 	//  8 -> 15
         -1,        -1,        -1,        -1,        28,        30,        -1,        -1, 	// 16 -> 23
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 24 -> 31
         28,        -1,        28,        -1,        -1,        -1,        -1,        -1,	// 32 -> 39
         -1,        -1,        28,        30,        28,        -1,        -1,        -1,	// 40 -> 47
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 48 -> 55
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 56 -> 63
} ;

static uint8_t gpioToClkDiv [] =
{
         -1,        -1,        -1,        -1,        29,        31,        33,        -1,	//  0 ->  7
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1, 	//  8 -> 15
         -1,        -1,        -1,        -1,        29,        31,        -1,        -1, 	// 16 -> 23
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 24 -> 31
         29,        -1,        29,        -1,        -1,        -1,        -1,        -1,	// 32 -> 39
         -1,        -1,        29,        31,        29,        -1,        -1,        -1,	// 40 -> 47
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 48 -> 55
         -1,        -1,        -1,        -1,        -1,        -1,        -1,        -1,	// 56 -> 63
} ;
/**
 * cubieboard use
 *
 *
 */
/* to get to right cfg reg */
unsigned int  gpioToCfg_CB [] =
{
  0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,
} ;


/* to reach the right bit for each pin */
unsigned int  gpioToShift_CB [] =
{
  0,4,8,12,16,20,24,28,
  0,4,8,12,16,20,24,28,
  0,4,8,12,16,20,24,28,
  0,4,8,12,16,20,24,28,
} ;

/* from PA - PI need fix this */
unsigned int  gpioToGPFSEL_CB [] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // for PD0~ PD27
  6,6,6,6,6,6,6,6,6,6,6,6, //for PE0~PE11
  2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,
} ;


static int physToGpio_CB[97] =
{
// U14 near by SATA	
 -1,   //0
 96,   -1,   //1    2:PD0  GND
 98,   97,   //3    4:PD2  PD1
100,   99,   //5    6:PD4  PD3
102,  101,   //7    8:PD6  PD5
 -1,  103,   //9   10:GND  PD7
105,  104,   //11  12:PD9  PD8
107,  106,   //13  14:PD11  PD10
109,  108,   //15  16:PD13  PD12
111,  110,   //17  18:PD15  PD14
112,   -1,   //19  20:PD16  GND
114,  113,   //21  22:PD18  PD17
116,  115,   //23  24:PD20  PD19
118,  117,   //25  26:PD22  PD21
123,  119,   //27  28:PD27  PD23
120,  122,   //29  30:PD24  PD26
 34,  121,   //31  32:PB2(PWM0)  PD25
 -1,   -1,   //33  34:YP_TP XP_TP
 -1,   -1,   //35  36:YN_TP XN_TP
231,   -1,   //37  38:PH7   GND
 43,   42,   //39  40:PB11  PB10
 -1,   -1,   //41  42:SPDIF  GND
 -1,   -1,   //43  44:5.5v   3.3v
266,   268,  //45  46:PI10  PI12
267,   269,  //47  48:PI11  PI13

//U15 near by USB
 -1,  239,   //1    2:5.5v   PH15
 -1,  238,   //3    4:2.8v   PH14
192,   50,   //5    6:PG0    PB18
 51,  195,   //7    8:PB19   PG3
194,  193,   //9   10:PG2    PG1
196,  197,   //11  12:PG4    PG5
198,  199,   //13  14:PG6    PG7
200,  201,   //15  16:PG8    PG9
202,  203,   //17  18:PG10   PG11
 -1,   -1,   //19  20:GND    GND
 -1,  260,   //21  22:FMINL  PI4
 -1,  261,   //23  24:FMINR  PI5
 -1,  262,   //25  26:GND    PI6
 -1,  263,   //27  28:VGA-R  PI7
 -1,  264,   //29  30:VGA-G  PI8
 -1,  265,   //31  32:VGA-B  PI9
 -1,  132,   //33  34:LCD1-VSYNC  PE4
 -1,  133,   //35  36:LCD1-HSYNC  PE5
 -1,  134,   //37  38:GND       PE6
 -1,  135,   //39  40:VCC       PE7
 -1,  136,   //41  42:LRADC0    PE8
 -1,  137,   //43  44:VCBS      PE9
 -1,  138,   //45  46:HPL       PE10
 -1,  139,   //47  48:HPR       PE11
};

static int CB_PIN_MASK[9][32] =  //[BANK]  [INDEX]
{
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PA
	{-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,10,11,-1,-1,-1,-1,-1,-1,18,19,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PB
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PC
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,-1,-1,-1,-1,},//PD
	{-1,-1,-1,-1, 4, 5, 6, 7, 8, 9,10,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PE
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PF
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PG
	{-1,-1,-1,-1,-1,-1,-1, 7,-1,-1,-1,-1,-1,-1,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PH
	{-1,-1,-1,-1, 4, 5, 6, 7, 8, 9,10,11,12,13,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PI
};

/*
 * Functions
 *********************************************************************************
 */
/**
 *
 * gootoomoon tools func
 *
 */
uint32_t readl(uint32_t addr)
{
  static volatile uint32_t *gpio_vaddr ;

  int fd = 0;
  uint32_t val = 0;
  fd = open("/dev/mem", O_RDWR | O_NDELAY);
  if(-1 == fd){
		printf("open /dev/mem failed.");
		return -1;
   }

  uint32_t mmap_base = (addr & ~MAP_MASK);
  uint32_t mmap_seek = ((addr - mmap_base) >> 2);

  gpio_vaddr = (uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_base);

  if (0 == gpio_vaddr){
	  close(fd);
	  printf("mmap failed:fd(%d), addr(0x%x),size(%d)\n", fd, addr, MAP_SIZE);
  } else {
	  val = *(gpio_vaddr + mmap_seek);
  		if (wiringPiDebug)
	 		printf("readl: vaddr = 0x%x,val:0x%x\n", (gpio_vaddr + mmap_seek),val);
	  close(fd);
	  munmap((uint32_t *)gpio_vaddr, MAP_SIZE);
	  return val;
  }
}

void writel(uint32_t val, uint32_t addr)
{
  static volatile uint32_t *gpio_vaddr ;

  int fd = 0;
  fd = open("/dev/mem", O_RDWR | O_NDELAY);
  if(-1 == fd){
		printf("open /dev/mem failed.");
		return -1;
   }

  uint32_t mmap_base = (addr & ~MAP_MASK);
  uint32_t mmap_seek = ((addr - mmap_base) >> 2);

  gpio_vaddr = (uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_base);

  if (0 == gpio_vaddr){
	  	close(fd);
  		if (wiringPiDebug)
	 		printf("mmap failed:fd(%d), addr(0x%x),size(%d)\n", fd, addr, MAP_SIZE);
  } else {
	  if (wiringPiDebug)
		  printf("write: vaddr = 0x%x, val to be set:0x%x", (gpio_vaddr + mmap_seek), val);
	  *(gpio_vaddr + mmap_seek) = val;
	  if (wiringPiDebug)
		  printf("   var = 0x%x\n", *(gpio_vaddr + mmap_seek));
	  close(fd);
	  munmap((uint32_t *)gpio_vaddr, MAP_SIZE);
  }
}



/*
 * wiringPiFailure:
 *	Fail. Or not.
 *********************************************************************************
 */

int wiringPiFailure (char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr,"%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}


/*
 * piBoardRev:
 *	Return a number representing the hardware revision of the board.
 *	Revision is currently 1 or 2. -1 is returned on error.
 *
 *	Much confusion here )-:
 *	Seems there are some boards with 0000 in them (mistake in manufacture)
 *	and some board with 0005 in them (another mistake in manufacture?)
 *	So the distinction between boards that I can see is:
 *	0000 - Error
 *	0001 - Not used
 *	0002 - Rev 1
 *	0003 - Rev 1
 *	0004 - Rev 2 (Early reports?
 *	0005 - Rev 2 (but error?)
 *	0006 - Rev 2
 *	0008 - Rev 2 - Model A
 *	000e - Rev 2 + 512MB
 *	000f - Rev 2 + 512MB
 *
 *	A small thorn is the olde style overvolting - that will add in
 *		1000000
 *
 *********************************************************************************
 */

static void piBoardRevOops (char *why)
{
  fprintf (stderr, "piBoardRev: Unable to determine board revision from /proc/cpuinfo\n") ;
  fprintf (stderr, " -> %s\n", why) ;
  fprintf (stderr, " ->  You may want to check:\n") ;
  fprintf (stderr, " ->  http://www.raspberrypi.org/phpBB3/viewtopic.php?p=184410#p184410\n") ;
  exit (EXIT_FAILURE) ;
}

int piBoardRev (void)
{
  FILE *cpuFd ;
  char line [120] ;
  char *c, lastChar ;
  static int  boardRev = -1 ;

  if (boardRev != -1)	// No point checking twice
    return boardRev ;

  if ((cpuFd = fopen ("/proc/cpuinfo", "r")) == NULL)
    piBoardRevOops ("Unable to open /proc/cpuinfo") ;

  while (fgets (line, 120, cpuFd) != NULL)
    if (strncmp (line, "Revision", 8) == 0)
      break ;

  fclose (cpuFd) ;

  if (strncmp (line, "Revision", 8) != 0)
    piBoardRevOops ("No \"Revision\" line") ;

  for (c = &line [strlen (line) - 1] ; (*c == '\n') || (*c == '\r') ; --c)
    *c = 0 ;
  
  if (wiringPiDebug)
    printf ("piboardRev: Revision string: %s\n", line) ;

  for (c = line ; *c ; ++c)
    if (isdigit (*c))
      break ;

  if (!isdigit (*c))
    piBoardRevOops ("No numeric revision string") ;

// If you have overvolted the Pi, then it appears that the revision
//	has 100000 added to it!

  if (wiringPiDebug)
    if (strlen (c) != 4)
      printf ("piboardRev: This Pi has/is overvolted!\n") ;

  lastChar = line [strlen (line) - 1] ;

  if (wiringPiDebug)
    printf ("piboardRev: lastChar is: '%c' (%d, 0x%02X)\n", lastChar, lastChar, lastChar) ;

  /**/ if ((lastChar == '2') || (lastChar == '3'))
    boardRev = 1 ;
  else
    boardRev = 2 ;

  if (wiringPiDebug)
    printf ("piBoardRev: Returning revision: %d\n", boardRev) ;

  return boardRev ;
}


/*
 * wpiPinToGpio:
 *	Translate a wiringPi Pin number to native GPIO pin number.
 *	Provided for external support.
 *********************************************************************************
 */

int wpiPinToGpio (int wpiPin)
{
  return pinToGpio [wpiPin & 63] ;
}


/*
 * physPinToGpio:
 *	Translate a physical Pin number to native GPIO pin number.
 *	Provided for external support.
 *********************************************************************************
 */

int physPinToGpio (int physPin)
{
  return physToGpio [physPin & 63] ;
}


/*
 * setPadDrive:
 *	Set the PAD driver value
 *********************************************************************************
 */

void setPadDrive (int group, int value)
{
  uint32_t wrVal ;

  if ((wiringPiMode == WPI_MODE_PINS) || (wiringPiMode == WPI_MODE_GPIO))
  {
    if ((group < 0) || (group > 2))
      return ;

    wrVal = BCM_PASSWORD | 0x18 | (value & 7) ;
    *(pads + group + 11) = wrVal ;

    if (wiringPiDebug)
    {
      printf ("setPadDrive: Group: %d, value: %d (%08X)\n", group, value, wrVal) ;
      printf ("Read : %08X\n", *(pads + group + 11)) ;
    }
  }
}


/*
 * getAlt:
 *	Returns the ALT bits for a given port. Only really of-use
 *	for the gpio readall command (I think)
 *********************************************************************************
 */

int getAlt (int pin)
{
  int fSel, shift, alt ;

  pin &= 63 ;

  /**/ if (wiringPiMode == WPI_MODE_PINS)
    pin = pinToGpio [pin] ;
  else if (wiringPiMode == WPI_MODE_PHYS)
    pin = physToGpio [pin] ;
  else if (wiringPiMode != WPI_MODE_GPIO)
    return 0 ;

  fSel    = gpioToGPFSEL [pin] ;
  shift   = gpioToShift  [pin] ;

  alt = (*(gpio + fSel) >> shift) & 7 ;

  return alt ;
}


/*
 * pwmSetMode:
 *	Select the native "balanced" mode, or standard mark:space mode
 *********************************************************************************
 */

void pwmSetMode (int mode)
{
#ifdef CB_BOARD
	sunxi_pwm_set_mode(mode);
#else //for PI	
  if ((wiringPiMode == WPI_MODE_PINS) || (wiringPiMode == WPI_MODE_GPIO) || (wiringPiMode == WPI_MODE_PHYS))
  {
    if (mode == PWM_MODE_MS)
      *(pwm + PWM_CONTROL) = PWM0_ENABLE | PWM1_ENABLE | PWM0_MS_MODE | PWM1_MS_MODE ;
    else
      *(pwm + PWM_CONTROL) = PWM0_ENABLE | PWM1_ENABLE ;
  }
#endif
}


/*
 * pwmSetRange:
 *	Set the PWM range register. We set both range registers to the same
 *	value. If you want different in your own code, then write your own.
 *********************************************************************************
 */


void pwmSetRange (unsigned int range)
{
#ifdef CB_BOARD
	//sunxi_pwm_set_period(range);

#else //for PI	
  if ((wiringPiMode == WPI_MODE_PINS) || (wiringPiMode == WPI_MODE_GPIO) || (wiringPiMode == WPI_MODE_PHYS))
  {
    *(pwm + PWM0_RANGE) = range ; delayMicroseconds (10) ;
    *(pwm + PWM1_RANGE) = range ; delayMicroseconds (10) ;
  }
#endif
}


/*
 * pwmSetClock:
 *	Set/Change the PWM clock. Originally my code, but changed
 *	(for the better!) by Chris Hall, <chris@kchall.plus.com>
 *	after further study of the manual and testing with a 'scope
 *********************************************************************************
 */

void pwmSetClock (int divisor)
{
#ifdef CB_BOARD
	sunxi_pwm_set_clk(divisor);
	sunxi_pwm_set_enable(1);
#else //for PI	
  uint32_t pwm_control ;
  divisor &= 4095 ;

  if ((wiringPiMode == WPI_MODE_PINS) || (wiringPiMode == WPI_MODE_GPIO) || (wiringPiMode == WPI_MODE_PHYS))
  {
    if (wiringPiDebug)
      printf ("Setting to: %d. Current: 0x%08X\n", divisor, *(clk + PWMCLK_DIV)) ;

    pwm_control = *(pwm + PWM_CONTROL) ;		// preserve PWM_CONTROL

// We need to stop PWM prior to stopping PWM clock in MS mode otherwise BUSY
// stays high.

    *(pwm + PWM_CONTROL) = 0 ;				// Stop PWM

// Stop PWM clock before changing divisor. The delay after this does need to
// this big (95uS occasionally fails, 100uS OK), it's almost as though the BUSY
// flag is not working properly in balanced mode. Without the delay when DIV is
// adjusted the clock sometimes switches to very slow, once slow further DIV
// adjustments do nothing and it's difficult to get out of this mode.

    *(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x01 ;	// Stop PWM Clock
      delayMicroseconds (110) ;			// prevents clock going sloooow

    while ((*(clk + PWMCLK_CNTL) & 0x80) != 0)	// Wait for clock to be !BUSY
      delayMicroseconds (1) ;

    *(clk + PWMCLK_DIV)  = BCM_PASSWORD | (divisor << 12) ;

    *(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x11 ;	// Start PWM clock
    *(pwm + PWM_CONTROL) = pwm_control ;		// restore PWM_CONTROL

    if (wiringPiDebug)
      printf ("Set     to: %d. Now    : 0x%08X\n", divisor, *(clk + PWMCLK_DIV)) ;
  }
#endif
}


/*
 * gpioClockSet:
 *	Set the freuency on a GPIO clock pin
 *********************************************************************************
 */

void gpioClockSet (int pin, int freq)
{
  int divi, divr, divf ;

  pin &= 63 ;

  /**/ if (wiringPiMode == WPI_MODE_PINS)
    pin = pinToGpio [pin] ;
  else if (wiringPiMode == WPI_MODE_PHYS)
    pin = physToGpio [pin] ;
  else if (wiringPiMode != WPI_MODE_GPIO)
    return ;
  
  divi = 19200000 / freq ;
  divr = 19200000 % freq ;
  divf = (int)((double)divr * 4096.0 / 19200000.0) ;

  if (divi > 4095)
    divi = 4095 ;

  *(clk + gpioToClkCon [pin]) = BCM_PASSWORD | GPIO_CLOCK_SOURCE ;		// Stop GPIO Clock
  while ((*(clk + gpioToClkCon [pin]) & 0x80) != 0)				// ... and wait
    ;

  *(clk + gpioToClkDiv [pin]) = BCM_PASSWORD | (divi << 12) | divf ;		// Set dividers
  *(clk + gpioToClkCon [pin]) = BCM_PASSWORD | 0x10 | GPIO_CLOCK_SOURCE ;	// Start Clock
}


/*
 * wiringPiFindNode:
 *      Locate our device node
 *********************************************************************************
 */

static struct wiringPiNodeStruct *wiringPiFindNode (int pin)
{
  struct wiringPiNodeStruct *node = wiringPiNodes ;

  while (node != NULL)
    if ((pin >= node->pinBase) && (pin <= node->pinMax))
      return node ;
    else
      node = node->next ;

  return NULL ;
}


/*
 * wiringPiNewNode:
 *	Create a new GPIO node into the wiringPi handling system
 *********************************************************************************
 */

static void pinModeDummy             (struct wiringPiNodeStruct *node, int pin, int mode)  { return ; }
static void pullUpDnControlDummy     (struct wiringPiNodeStruct *node, int pin, int pud)   { return ; }
static int  digitalReadDummy         (struct wiringPiNodeStruct *node, int pin)            { return LOW ; }
static void digitalWriteDummy        (struct wiringPiNodeStruct *node, int pin, int value) { return ; }
static void pwmWriteDummy            (struct wiringPiNodeStruct *node, int pin, int value) { return ; }
static int  analogReadDummy          (struct wiringPiNodeStruct *node, int pin)            { return 0 ; }
static void analogWriteDummy         (struct wiringPiNodeStruct *node, int pin, int value) { return ; }

struct wiringPiNodeStruct *wiringPiNewNode (int pinBase, int numPins)
{
  int    pin ;
  struct wiringPiNodeStruct *node ;

// Minimum pin base is 64

  if (pinBase < 64)
    (void)wiringPiFailure ("wiringPiNewNode: pinBase of %d is < 64\n", pinBase) ;

// Check all pins in-case there is overlap:

  for (pin = pinBase ; pin < (pinBase + numPins) ; ++pin)
    if (wiringPiFindNode (pin) != NULL)
      (void)wiringPiFailure ("wiringPiNewNode: Pin %d overlaps with existing definition\n", pin) ;

  node = calloc (sizeof (struct wiringPiNodeStruct), 1) ;	// calloc zeros
  if (node == NULL)
    (void)wiringPiFailure ("wiringPiNewNode: Unable to allocate memory: %s\n", strerror (errno)) ;

  node->pinBase         = pinBase ;
  node->pinMax          = pinBase + numPins - 1 ;
  node->pinMode         = pinModeDummy ;
  node->pullUpDnControl = pullUpDnControlDummy ;
  node->digitalRead     = digitalReadDummy ;
  node->digitalWrite    = digitalWriteDummy ;
  node->pwmWrite        = pwmWriteDummy ;
  node->analogRead      = analogReadDummy ;
  node->analogWrite     = analogWriteDummy ;
  node->next            = wiringPiNodes ;
  wiringPiNodes         = node ;

  return node ;
}


#ifdef notYetReady
/*
 * pinED01:
 * pinED10:
 *	Enables edge-detect mode on a pin - from a 0 to a 1 or 1 to 0
 *	Pin must already be in input mode with appropriate pull up/downs set.
 *********************************************************************************
 */

void pinEnableED01Pi (int pin)
{
  pin = pinToGpio [pin & 63] ;
}
#endif
/*
 * cubieboard func
 */

void sunxi_pwm_set_enable(int en)
{
	int val = 0;
	val = readl(SUNXI_PWM_CTRL_REG);
	if(en){
		//val |= (SUNXI_PWM_CH0_EN | SUNXI_PWM_CH1_EN);
		//val |= (SUNXI_PWM_SCLK_CH0_GATING | SUNXI_PWM_SCLK_CH1_GATING);
		val |= (SUNXI_PWM_CH0_EN | SUNXI_PWM_SCLK_CH0_GATING);
	} else {
		//val &= ~(SUNXI_PWM_CH0_EN | SUNXI_PWM_CH1_EN);
		//val &= ~(SUNXI_PWM_SCLK_CH0_GATING | SUNXI_PWM_SCLK_CH1_GATING);
		val &= ~(SUNXI_PWM_CH0_EN | SUNXI_PWM_SCLK_CH0_GATING);
	}
  if (wiringPiDebug)
	printf(">>func：%s,no:%d,enable? :0x%x\n",__func__, __LINE__, val);
	writel(val, SUNXI_PWM_CTRL_REG);
}

void sunxi_pwm_set_mode(int mode)
{
	int val = 0;
	val = readl(SUNXI_PWM_CTRL_REG);
	mode &= 1; //cover the mode to 0 or 1
	if(mode){ //pulse mode
		val |= (SUNXI_PWM_CH0_MS_MODE | SUNXI_PWM_CH1_MS_MODE);
	} else {  //cycle mode
		val &= ~(SUNXI_PWM_CH0_MS_MODE | SUNXI_PWM_CH1_MS_MODE);
	}

	val |= (SUNXI_PWM_CH0_ACT_STA | SUNXI_PWM_CH1_ACT_STA);
 	 if (wiringPiDebug)
		printf(">>func：%s,no:%d,mode? :0x%x\n",__func__, __LINE__, val);
	writel(val, SUNXI_PWM_CTRL_REG);
}

void sunxi_pwm_set_clk(int clk)
{
	int val = 0;
	int clk_val = 0;
//	sunxi_pwm_set_enable(0);
	val = readl(SUNXI_PWM_CTRL_REG);
	//clear clk to 0
	val &= 0xf801f0;

	//set both ch clk to needed
	//val |= ((clk & 0xf) | ((clk & 0xf) << 15));  //todo check wether clk is invalid or not
	
	val |= (clk & 0xf);  //todo check wether clk is invalid or not

	writel(val, SUNXI_PWM_CTRL_REG);
	sunxi_pwm_set_enable(1);
  	if (wiringPiDebug)
		printf(">>func：%s,no:%d,clk? :0x%x\n",__func__, __LINE__, val);
}
/**
 * ch0 and ch1 set the same
 */
uint32_t sunxi_pwm_get_period(void)
{
	uint32_t period_cys = 0;
	period_cys = readl(SUNXI_PWM_CH0_PERIOD);//get ch0 period_cys
	period_cys &= 0xffff0000;//get period_cys
	period_cys = period_cys >> 16;
  	if (wiringPiDebug)
		printf(">>func:%s,no:%d,period/range:%d",__func__,__LINE__,period_cys);
	return period_cys;
}

void sunxi_pwm_set_period(int period_cys)
{
	uint32_t val = 0;
	//all clear to 0
  	if (wiringPiDebug)
		printf(">>func:%s no:%d\n",__func__,__LINE__);
//	writel(0,SUNXI_PWM_CH0_PERIOD);
//	writel(0,SUNXI_PWM_CH1_PERIOD);
	period_cys &= 0xff; //set max period to 256

	period_cys = period_cys << 16;
	val = sunxi_pwm_get_period();
	val &=0xff;
	period_cys |= val;
/*
	int val = readl(SUNXI_PWM_CTRL_REG);
	val &= ~(SUNXI_PWM_SCLK_CH0_GATING | SUNXI_PWM_SCLK_CH1_GATING);
	writel(val,SUNXI_PWM_CTRL_REG);
*/
	writel(period_cys, SUNXI_PWM_CH0_PERIOD);
//	writel(period_cys, SUNXI_PWM_CH1_PERIOD);

//	val |= (SUNXI_PWM_SCLK_CH0_GATING | SUNXI_PWM_SCLK_CH1_GATING);
//	writel(val,SUNXI_PWM_CTRL_REG);
}

void sunxi_pwm_set_act(int act_cys)
{
	uint32_t per0 = 0;
	uint32_t per1 = 0;
	//keep period the same, clear act_cys to 0 first
  	if (wiringPiDebug)
		printf(">>func:%s no:%d\n",__func__,__LINE__);
	per0 = readl(SUNXI_PWM_CH0_PERIOD);
	per0 &= 0xff0000;
//	per1 = readl(SUNXI_PWM_CH1_PERIOD);
//	per1 &= 0xff0000;

	//set act and period keep befor
	act_cys &= 0xff;
	//disable time gate
/*	int val = readl(SUNXI_PWM_CTRL_REG);
	val &= ~(SUNXI_PWM_SCLK_CH0_GATING | SUNXI_PWM_SCLK_CH1_GATING);
	writel(val,SUNXI_PWM_CTRL_REG);
*/
	act_cys |= per0;
	writel(act_cys,SUNXI_PWM_CH0_PERIOD);
//	writel((per1 | act_cys),SUNXI_PWM_CH1_PERIOD);

//	val |= (SUNXI_PWM_SCLK_CH0_GATING | SUNXI_PWM_SCLK_CH1_GATING);
//	printf(">>>>>>>>>>>>>> func:%s no:%d\n",__func__,__LINE__);
//	writel(val,SUNXI_PWM_CTRL_REG);
//	printf("<<<<<<<<<<<<<< func:%s no:%d\n",__func__,__LINE__);
}

void sunxi_set_gpio_mode(int pin,int mode)
{
	uint32_t regval = 0;
	uint32_t regval_ch = 0;
	int bank = pin >> 5;
	int index = pin - (bank << 5);
	int offset = ((index - ((index >> 3) << 3)) << 2);
	uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + ((index >> 3) << 2);
  	if (wiringPiDebug)
		printf("func:%s pin:%d, MODE:%d bank:%d index:%d phyaddr:0x%x\n",__func__, pin , mode,bank,index,phyaddr);	
	if(CB_PIN_MASK[bank][index] != -1){
		regval = readl(phyaddr);
  		if (wiringPiDebug)
			printf("read reg val: 0x%x offset:%d\n",regval,offset);
		if(INPUT == mode){
			regval &= ~(7 << offset);
			writel(regval, phyaddr);
			regval = readl(phyaddr);
  			if (wiringPiDebug)
				printf("Input mode set over reg val: 0x%x\n",regval);
		} else if(OUTPUT == mode){
			regval &= ~(7 << offset);
			regval |=  (1 << offset);
  			if (wiringPiDebug)
				printf("Out mode ready set val: 0x%x\n",regval);
			writel(regval, phyaddr);
			regval = readl(phyaddr);
  			if (wiringPiDebug)
				printf("Out mode set over reg val: 0x%x\n",regval);
		} else if(PWM_OUTPUT == mode){
			// set pin PWMx to pwm mode
			regval &= ~(7 << offset);
			regval |=  (0x2 << offset);
  			if (wiringPiDebug)
				printf(">>>>>line:%d PWM mode ready to set val: 0x%x\n",__LINE__,regval);
			writel(regval, phyaddr);
			delayMicroseconds (200);
			regval = readl(phyaddr);
  			if (wiringPiDebug)
				printf("<<<<<PWM mode set over reg val: 0x%x\n",regval);

			//clear all reg
			writel(0,SUNXI_PWM_CTRL_REG);	
			writel(0,SUNXI_PWM_CH0_PERIOD);	
			writel(0,SUNXI_PWM_CH1_PERIOD);	


			//set default M:S to 1/2
			sunxi_pwm_set_period(255);
			sunxi_pwm_set_act(128);
			pwmSetMode(PWM_MODE_MS);
			sunxi_pwm_set_clk(PWM_CLK_DIV_120);//default clk:24M/120
			delayMicroseconds (200);
		}
	} else {
		printf("line:%dpin number error\n",__LINE__);
	}	
}

sunxi_digitalWrite(int pin, int value)
{	
	uint32_t regval = 0;
	int bank = pin >> 5;
	int index = pin - (bank << 5);
	uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + 0x10; // +0x10 -> data reg
  	if (wiringPiDebug)
		printf("func:%s pin:%d, value:%d bank:%d index:%d phyaddr:0x%x\n",__func__, pin , value,bank,index,phyaddr);	
	if(CB_PIN_MASK[bank][index] != -1){
		regval = readl(phyaddr);
  		if (wiringPiDebug)
			printf("befor write reg val: 0x%x,index:%d\n",regval,index);
		if(0 == value){
			regval &= ~(1 << index);
			writel(regval, phyaddr);
			regval = readl(phyaddr);
  			if (wiringPiDebug)
				printf("LOW val set over reg val: 0x%x\n",regval);
		} else {
			regval |= (1 << index);
			writel(regval, phyaddr);
			regval = readl(phyaddr);
  			if (wiringPiDebug)
				printf("HIGH val set over reg val: 0x%x\n",regval);
		}

	} else {
		printf("pin number error\n");
	}	
}

int sunxi_digitalRead(int pin)
{	
	uint32_t regval = 0;
	int bank = pin >> 5;
	int index = pin - (bank << 5);
	uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + 0x10; // +0x10 -> data reg
  	if (wiringPiDebug)
		printf("func:%s pin:%d,bank:%d index:%d phyaddr:0x%x\n",__func__, pin,bank,index,phyaddr);	
	if(CB_PIN_MASK[bank][index] != -1){
		regval = readl(phyaddr);
		regval = regval >> index;
		regval &= 1;
  		if (wiringPiDebug)
			printf("***** read reg val: 0x%x,bank:%d,index:%d,line:%d\n",regval,bank,index,__LINE__);
		return regval;
	} else {
		printf("pin number error\n");
	}	
}


void sunxi_pullUpDnControl (int pin, int pud)
{
	uint32_t regval = 0;
	int bank = pin >> 5;
	int index = pin - (bank << 5);
	int sub = index >> 4;
	int sub_index = index - 16*sub;
	uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + 0x1c + sub; // +0x10 -> pullUpDn reg
  	if (wiringPiDebug)
		printf("func:%s pin:%d,bank:%d index:%d sub:%d phyaddr:0x%x\n",__func__, pin,bank,index,sub,phyaddr);	
	if(CB_PIN_MASK[bank][index] != -1){  //PI13~PI21 need check again
		regval = readl(phyaddr);
  		if (wiringPiDebug)
			printf("pullUpDn reg:0x%x, pud:0x%x sub_index:%d\n", regval, pud, sub_index);
		regval &= ~(3 << (sub_index << 1));
		regval |= (pud << (sub_index << 1));
  		if (wiringPiDebug)
			printf("pullUpDn val ready to set:0x%x\n", regval);

		writel(regval, phyaddr);

		regval = readl(phyaddr);
  		if (wiringPiDebug)
			printf("pullUpDn reg after set:0x%x  addr:0x%x\n", regval, phyaddr);
	} else {
		printf("pin number error\n");
	}	
	
}
/*
 *********************************************************************************
 * Core Functions
 *********************************************************************************
 */


/*
 * pinMode:
 *	Sets the mode of a pin to be input, output or PWM output
 *********************************************************************************
 */

//void pinMode (int pin, int mode)
int pinMode (int pin, int mode)
{
  int    fSel, shift, alt;
  struct wiringPiNodeStruct *node = wiringPiNodes ;
	wiringPiSetupPhys();
  	if (wiringPiDebug)
  		printf ("%s,%d,pin:%d,mode:%d\n", __func__, __LINE__,pin,mode) ;

  if ((pin < MAX_PIN_NUM))// On-board pin  OR extral model pin
  {
	  if (wiringPiMode == WPI_MODE_PINS){
#ifndef CB_BOARD
		  pin = pinToGpio [pin] ;
#endif

  		if (wiringPiDebug)
		  	printf ("WPI_MODE_PINS  %s,%d,pin:%d\n", __func__, __LINE__,pin);
	  }
	  else if (wiringPiMode == WPI_MODE_PHYS){
#ifdef CB_BOARD
		  pin = physToGpio_CB[pin] ;
  		if (wiringPiDebug)
		  	printf ("PHY_MODE_PINS  %s,%d,inter_pin:%d\n", __func__, __LINE__,pin);
		  if(-1 == pin){
			printf("invalid pin,please check it over.\n");
			return;
		  }
#else
		  pin = physToGpio [pin] ;
#endif
	  } else if (wiringPiMode != WPI_MODE_GPIO)
		  return ;
#ifndef CB_BOARD
	  fSel    = gpioToGPFSEL [pin] ;
	  shift   = gpioToShift  [pin] ;
#endif

	if (mode == INPUT){
		/* set pin to INPUT mode */
#ifdef CB_BOARD
		sunxi_set_gpio_mode(pin,INPUT);
		wiringPinMode = INPUT;
		return 0;
#else
		*(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) ; // Sets bits to zero = input
#endif
	} else if (mode == OUTPUT){
	
#ifdef CB_BOARD
		/* set pin to OUTPUT mode */
		
		sunxi_set_gpio_mode(pin, OUTPUT); //gootoomoon_set_mode
		wiringPinMode = OUTPUT;
		return 0;
#else
		*(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift) ;
	
#endif
	}
	else if (mode == PWM_OUTPUT)
	{
#ifdef CB_BOARD
		if((pin != 34) && (pin != 259)){
			printf("the pin you choose is not surport hardware PWM\n");
			printf("you can select PB2 or PI3 for PWM pin\n");
			printf("or you can use it in softPwm mode\n");
			return ;
		}
		printf("you choose the hardware PWM:%d\n", pin);
		sunxi_set_gpio_mode(pin,PWM_OUTPUT);
		wiringPinMode = PWM_OUTPUT;
		return 0;
#else
		if ((alt = gpioToPwmALT [pin]) == 0)	// Not a PWM pin
			return ;
		// Set pin to PWM mode

		*(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (alt << shift) ;
		delayMicroseconds (110) ;		// See comments in pwmSetClockWPi

		pwmSetMode  (PWM_MODE_BAL) ;	// Pi default mode
		pwmSetRange (1024) ;		// Default range of 1024
		pwmSetClock (32) ;			// 19.2 / 32 = 600KHz - Also starts the PWM
#endif
	}
    	else if (mode == PULLUP)
    	{
		pullUpDnControl (pin, 1);
		wiringPinMode = PULLUP;
		return 0;
	}
    	else if (mode == PULLDOWN)
    	{
		pullUpDnControl (pin, 2);
		wiringPinMode = PULLDOWN;
		return 0;
	}
    	else if (mode == PULLOFF)
    	{
		pullUpDnControl (pin, 0);
		wiringPinMode = PULLOFF;
		return 0;
	}
    	else if (mode == CHECK)
    	{
		return wiringPinMode;
	}
#if 0  // no need for cubieboard
    	else if (mode == GPIO_CLOCK)
    	{
      		if ((alt = gpioToGpClkALT0 [pin]) == 0)	// Not a GPIO_CLOCK pin
		return ;

		// Set pin to GPIO_CLOCK mode and set the clock frequency to 100KHz

      		*(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (alt << shift) ;
      		delayMicroseconds (110) ;
      		gpioClockSet      (pin, 100000) ;
    	}

#endif
  }
  else //pin not on board
  {
  	if (wiringPiDebug)
    	printf ("start to find note %s,%d\n", __func__, __LINE__) ;
    if ((node = wiringPiFindNode (pin)) != NULL)
	  printf ("pinMode :%s,%d,%d\n", __func__, __LINE__, pin) ;
      node->pinMode (node, pin, mode) ;
    return ;
  }
}


/*
 * pullUpDownCtrl:
 *	Control the internal pull-up/down resistors on a GPIO pin
 *	The Arduino only has pull-ups and these are enabled by writing 1
 *	to a port when in input mode - this paradigm doesn't quite apply
 *	here though.
 *********************************************************************************
 */

void pullUpDnControl (int pin, int pud)
{
  	struct wiringPiNodeStruct *node = wiringPiNodes ;
#ifdef CB_BOARD
	if (pin < MAX_PIN_NUM)		// On-Board Pin
	{
		if (wiringPiMode == WPI_MODE_PINS)
			pin = physToGpio_CB [pin] ;
		else if (wiringPiMode == WPI_MODE_PHYS)
			pin = physToGpio_CB [pin] ;
		else if (wiringPiMode != WPI_MODE_GPIO)
			return ;

		pud &= 3 ;

		sunxi_pullUpDnControl(pin, pud);
	}
#else	

  if ((pin & PI_GPIO_MASK) == 0)		// On-Board Pin
  {
    /**/ if (wiringPiMode == WPI_MODE_PINS)
      pin = pinToGpio [pin] ;
    else if (wiringPiMode == WPI_MODE_PHYS)
      pin = physToGpio [pin] ;
    else if (wiringPiMode != WPI_MODE_GPIO)
      return ;

    pud &= 3 ;

    *(gpio + GPPUD)              = pud ;		delayMicroseconds (5) ;
    *(gpio + gpioToPUDCLK [pin]) = 1 << (pin & 31) ;	delayMicroseconds (5) ;
    
    *(gpio + GPPUD)              = 0 ;			delayMicroseconds (5) ;
    *(gpio + gpioToPUDCLK [pin]) = 0 ;			delayMicroseconds (5) ;
  }
#endif
  else
  {
    if ((node = wiringPiFindNode (pin)) != NULL)
      node->pullUpDnControl (node, pin, pud) ;
    return ;
  }
}


/*
 * digitalRead:
 *	Read the value of a given Pin, returning HIGH or LOW
 *********************************************************************************
 */

int digitalRead (int pin)
{
  char c ;
  struct wiringPiNodeStruct *node = wiringPiNodes ;

  if (pin < MAX_PIN_NUM)		// On-Board Pin
  {
    /**/ if (wiringPiMode == WPI_MODE_GPIO_SYS)	// Sys mode
    {
      if (sysFds [pin] == -1)
	return LOW ;

      lseek (sysFds [pin], 0L, SEEK_SET) ;
      read  (sysFds [pin], &c, 1) ;
      return (c == '0') ? LOW : HIGH ;
    }
    else if (wiringPiMode == WPI_MODE_PINS){
#ifdef CB_BOARD
		;
#else
      pin = pinToGpio [pin] ;
#endif
	} else if (wiringPiMode == WPI_MODE_PHYS){
#ifdef CB_BOARD
		  pin = physToGpio_CB [pin] ;
		  if(-1 == pin){
			printf("invalid pin,please check it over.\n");
			return;
		  }
#else
      pin = physToGpio [pin] ;
#endif
	}else if (wiringPiMode != WPI_MODE_GPIO)
      return LOW ;
#ifdef CB_BOARD
	sunxi_digitalRead(pin);
#else
    if ((*(gpio + gpioToGPLEV [pin]) & (1 << (pin & 31))) != 0)
      return HIGH ;
    else
      return LOW ;
#endif
  }
  else
  {
    if ((node = wiringPiFindNode (pin)) == NULL)
      return LOW ;
    return node->digitalRead (node, pin) ;
  }
}


/*
 * digitalWrite:
 *	Set an output bit
 *********************************************************************************
 */

void digitalWrite (int pin, int value)
{
	struct wiringPiNodeStruct *node = wiringPiNodes ;

  if (wiringPiDebug)
		printf ("%s,%d\n", __func__, __LINE__) ;

	if (pin < MAX_PIN_NUM)		// On-Board Pin needto fix me gootoomoon
	{
		if (wiringPiMode == WPI_MODE_GPIO_SYS)	// Sys mode
		{
  			if (wiringPiDebug)
	        	printf ("start to sysFds[%d]%s,%d\n",pin,  __func__, __LINE__) ;
			if (sysFds [pin] != -1)
			{
  				if (wiringPiDebug)
					printf ("start to write val: %s,%d\n", __func__, __LINE__) ;
				if (value == LOW){
					write (sysFds [pin], "0\n", 2) ;
  					if (wiringPiDebug)
						printf ("set LOW OK %s,%d\n", __func__, __LINE__) ;
				} else {
					write (sysFds [pin], "1\n", 2) ;
  					if (wiringPiDebug)
						printf ("set HIGH OK %s,%d\n", __func__, __LINE__) ;
				}
			} else {
				printf ("sysFds open faile: %s,%d\n", __func__, __LINE__) ;
			}

  			if (wiringPiDebug)
				printf ("seems OK %s,%d\n", __func__, __LINE__) ;
			return; //add by gootoomoon

		}
		else if (wiringPiMode == WPI_MODE_PINS){
#ifndef CB_BOARD
			pin = pinToGpio [pin] ;
#else
		;
#endif
		} else if (wiringPiMode == WPI_MODE_PHYS){
#ifdef CB_BOARD
		  pin = physToGpio_CB [pin] ;
		  if(-1 == pin){
			printf("invalid pin,please check it over.\n");
			return;
		  }
#else
			pin = physToGpio [pin] ;
#endif
		} else if (wiringPiMode != WPI_MODE_GPIO)
			return ;
	#ifdef CB_BOARD
		sunxi_digitalWrite(pin, value);
	#else
		if (value == LOW){
			//*(gpio + gpioToGPCLR [pin]) = 1 << (pin & 31) ;
  			if (wiringPiDebug)
				printf ("gpio vaddr: 0x%x, val: %x\n", gpio + 0x4 ,*(gpio + 0x4)) ;
			*(gpio + 0x4) = *(gpio + 0x4) & ~(1 << (pin & 31)) ; //for PDx data reg
  			if (wiringPiDebug)
				printf ("LOW over  val:%x\n",*(gpio +0x4)) ;
		}
		else {
  			if (wiringPiDebug)
				printf ("gpio vaddr: 0x%x, val: %x\n", gpio + 0x4 ,*(gpio + 0x4)) ;
			//*(gpio + gpioToGPSET [pin]) = 1 << (pin & 31) ;
			*(gpio + 0x4) = *(gpio + 0x4) | (1 << (pin & 31)) ;
  			if (wiringPiDebug)
				printf ("HIGH over  val:%x\n",*(gpio + 0x4)) ;
		}
	#endif
	}
	else
	{
	    printf ("not on board :%s,%d\n", __func__, __LINE__) ;
		if ((node = wiringPiFindNode (pin)) != NULL)
  			if (wiringPiDebug)
            	printf ("gootoomoon find node%s,%d\n", __func__, __LINE__) ;
			node->digitalWrite (node, pin, value) ;
	}

  	if (wiringPiDebug)
		printf ("this fun is ok now %s,%d\n", __func__, __LINE__) ;
}


/*
 * pwmWrite:
 *	Set an output PWM value
 *********************************************************************************
 */

void pwmWrite (int pin, int value)
{

  struct wiringPiNodeStruct *node = wiringPiNodes ;
#ifdef CB_BOARD
	uint32_t a_val = 0;
	uint32_t m_val = 0;
	if (pin < MAX_PIN_NUM)		// On-Board Pin needto fix me gootoomoon
	{
		if (wiringPiMode == WPI_MODE_PINS)
			pin = pinToGpio [pin] ;
		else if (wiringPiMode == WPI_MODE_PHYS){
			pin = physToGpio_CB[pin] ;
		} else if (wiringPiMode != WPI_MODE_GPIO)
			return ;

		if((pin != 34) && (pin != 259)){
			printf("please use soft pwmmode or choose PWM pin\n");
			return ;
		}
		a_val = sunxi_pwm_get_period();
  		if (wiringPiDebug)
			printf("==> no:%d period now is :%d,act_val to be set:%d\n",__LINE__,a_val, value);
	 	if(value > a_val){
			printf("val pwmWrite 0 <= X <= 255\n");
			printf("Or you can set new range by yourself by pwmSetRange(range\n");
			return;
		}
		//if value changed chang it
		sunxi_pwm_set_enable(0);
		sunxi_pwm_set_act(value);
		sunxi_pwm_set_enable(1);

	} else {
		printf ("not on board :%s,%d\n", __func__, __LINE__) ;
		if ((node = wiringPiFindNode (pin)) != NULL){
  			if (wiringPiDebug)
            	printf ("gootoomoon find node%s,%d\n", __func__, __LINE__) ;
			node->digitalWrite (node, pin, value) ;
		}
	}

  	if (wiringPiDebug)
		printf ("this fun is ok now %s,%d\n", __func__, __LINE__) ;

	
#else

  if ((pin & PI_GPIO_MASK) == 0)		// On-Board Pin
  {
    /**/ if (wiringPiMode == WPI_MODE_PINS)
      pin = pinToGpio [pin] ;
    else if (wiringPiMode == WPI_MODE_PHYS)
      pin = physToGpio [pin] ;
    else if (wiringPiMode != WPI_MODE_GPIO)
      return ;

    *(pwm + gpioToPwmPort [pin]) = value ;
  }
  else
  {
    if ((node = wiringPiFindNode (pin)) != NULL)
      node->pwmWrite (node, pin, value) ;
  }
#endif
}


/*
 * analogRead:
 *	Read the analog value of a given Pin. 
 *	There is no on-board Pi analog hardware,
 *	so this needs to go to a new node.
 *********************************************************************************
 */

int analogRead (int pin)
{
  struct wiringPiNodeStruct *node = wiringPiNodes ;

  if ((node = wiringPiFindNode (pin)) == NULL)
    return 0 ;
  else
    return node->analogRead (node, pin) ;
}


/*
 * analogWrite:
 *	Write the analog value to the given Pin. 
 *	There is no on-board Pi analog hardware,
 *	so this needs to go to a new node.
 *********************************************************************************
 */

void analogWrite (int pin, int value)
{
  struct wiringPiNodeStruct *node = wiringPiNodes ;

  if ((node = wiringPiFindNode (pin)) == NULL)
    return ;

  node->analogWrite (node, pin, value) ;
}



/*
 * digitalWriteByte:
 *	Pi Specific
 *	Write an 8-bit byte to the first 8 GPIO pins - try to do it as
 *	fast as possible.
 *	However it still needs 2 operations to set the bits, so any external
 *	hardware must not rely on seeing a change as there will be a change 
 *	to set the outputs bits to zero, then another change to set the 1's
 *********************************************************************************
 */

void digitalWriteByte (int value)
{
  uint32_t pinSet = 0 ;
  uint32_t pinClr = 0 ;
  int mask = 1 ;
  int pin ;

  /**/ if (wiringPiMode == WPI_MODE_GPIO_SYS)
  {
    for (pin = 0 ; pin < 8 ; ++pin)
    {
      digitalWrite (pin, value & mask) ;
      mask <<= 1 ;
    }
  }
  else
  {
    for (pin = 0 ; pin < 8 ; ++pin)
    {
      if ((value & mask) == 0)
	pinClr |= (1 << pinToGpio [pin]) ;
      else
	pinSet |= (1 << pinToGpio [pin]) ;

      mask <<= 1 ;
    }

    *(gpio + gpioToGPCLR [0]) = pinClr ;
    *(gpio + gpioToGPSET [0]) = pinSet ;
  }
}


/*
 * waitForInterrupt:
 *	Pi Specific.
 *	Wait for Interrupt on a GPIO pin.
 *	This is actually done via the /sys/class/gpio interface regardless of
 *	the wiringPi access mode in-use. Maybe sometime it might get a better
 *	way for a bit more efficiency.
 *********************************************************************************
 */

int waitForInterrupt (int pin, int mS)
{
  int fd, x ;
  uint8_t c ;
  struct pollfd polls ;

  /**/ if (wiringPiMode == WPI_MODE_PINS)
    pin = pinToGpio [pin] ;
  else if (wiringPiMode == WPI_MODE_PHYS)
    pin = physToGpio [pin] ;

  if ((fd = sysFds [pin & 63]) == -1)
    return -2 ;

// Setup poll structure

  polls.fd     = fd ;
  polls.events = POLLPRI ;	// Urgent data!

// Wait for it ...

  x = poll (&polls, 1, mS) ;

// Do a dummy read to clear the interrupt
//	A one character read appars to be enough.

  (void)read (fd, &c, 1) ;

  return x ;
}


/*
 * interruptHandler:
 *	This is a thread and gets started to wait for the interrupt we're
 *	hoping to catch. It will call the user-function when the interrupt
 *	fires.
 *********************************************************************************
 */

static void *interruptHandler (void *arg)
{
  int myPin = *(int *)arg ;

  (void)piHiPri (55) ;	// Only effective if we run as root

  for (;;)
    if (waitForInterrupt (myPin, -1) > 0)
      isrFunctions [myPin] () ;

  return NULL ;
}


/*
 * wiringPiISR:
 *	Pi Specific.
 *	Take the details and create an interrupt handler that will do a call-
 *	back to the user supplied function.
 *********************************************************************************
 */

int wiringPiISR (int pin, int mode, void (*function)(void))
{
  pthread_t threadId ;
  char fName   [64] ;
  char *modeS ;
  char  pinS [8] ;
  pid_t pid ;
  int   count, i ;
  uint8_t c ;

  pin &= 63 ;

  /**/ if (wiringPiMode == WPI_MODE_UNINITIALISED)
    (void)wiringPiFailure ("wiringPiISR: wiringPi has not been initialised. Unable to continue.\n") ;
  else if (wiringPiMode == WPI_MODE_PINS)
    pin = pinToGpio [pin] ;
  else if (wiringPiMode == WPI_MODE_PHYS)
    pin = physToGpio [pin] ;

// Now export the pin and set the right edge
//	We're going to use the gpio program to do this, so it assumes
//	a full installation of wiringPi. It's a bit 'clunky', but it
//	is a way that will work when we're running in "Sys" mode, as
//	a non-root user. (without sudo)

  if (mode != INT_EDGE_SETUP)
  {
    /**/ if (mode == INT_EDGE_FALLING)
      modeS = "falling" ;
    else if (mode == INT_EDGE_RISING)
      modeS = "rising" ;
    else
      modeS = "both" ;

    sprintf (pinS, "%d", pin) ;

    if ((pid = fork ()) < 0)	// Fail
      return pid ;

    if (pid == 0)	// Child, exec
    {
      execl ("/usr/local/bin/gpio", "gpio", "edge", pinS, modeS, (char *)NULL) ;
      return -1 ;	// Failure ...
    }
    else		// Parent, wait
      wait (NULL) ;
  }

// Now pre-open the /sys/class node - it may already be open if
//	we are in Sys mode, but this will do no harm.

  sprintf (fName, "/sys/class/gpio/gpio%d/value", pin) ;
  if ((sysFds [pin] = open (fName, O_RDWR)) < 0)
    return -1 ;

// Clear any initial pending interrupt

  ioctl (sysFds [pin], FIONREAD, &count) ;
  for (i = 0 ; i < count ; ++i)
    read (sysFds [pin], &c, 1) ;

  isrFunctions [pin] = function ;

  pthread_create (&threadId, NULL, interruptHandler, &pin) ;

  delay (1) ;

  return 0 ;
}


/*
 * initialiseEpoch:
 *	Initialise our start-of-time variable to be the current unix
 *	time in milliseconds and microseconds.
 *********************************************************************************
 */

static void initialiseEpoch (void)
{
  struct timeval tv ;

  gettimeofday (&tv, NULL) ;
  epochMilli = (uint64_t)tv.tv_sec * (uint64_t)1000    + (uint64_t)(tv.tv_usec / 1000) ;
  epochMicro = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)(tv.tv_usec) ;
}


/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay (unsigned int howLong)
{
  struct timespec sleeper, dummy ;

  sleeper.tv_sec  = (time_t)(howLong / 1000) ;
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

  nanosleep (&sleeper, &dummy) ;
}


/*
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicrosecondsHard (unsigned int howLong)
{
  struct timeval tNow, tLong, tEnd ;

  gettimeofday (&tNow, NULL) ;
  tLong.tv_sec  = howLong / 1000000 ;
  tLong.tv_usec = howLong % 1000000 ;
  timeradd (&tNow, &tLong, &tEnd) ;

  while (timercmp (&tNow, &tEnd, <))
    gettimeofday (&tNow, NULL) ;
}

void delayMicroseconds (unsigned int howLong)
{
  struct timespec sleeper ;

  /**/ if (howLong ==   0)
    return ;
  else if (howLong  < 100)
    delayMicrosecondsHard (howLong) ;
  else
  {
    sleeper.tv_sec  = 0 ;
    sleeper.tv_nsec = (long)(howLong * 1000) ;
    nanosleep (&sleeper, NULL) ;
  }
}


/*
 * millis:
 *	Return a number of milliseconds as an unsigned int.
 *********************************************************************************
 */

unsigned int millis (void)
{
  struct timeval tv ;
  uint64_t now ;

  gettimeofday (&tv, NULL) ;
  now  = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ;

  return (uint32_t)(now - epochMilli) ;
}


/*
 * micros:
 *	Return a number of microseconds as an unsigned int.
 *********************************************************************************
 */

unsigned int micros (void)
{
  struct timeval tv ;
  uint64_t now ;

  gettimeofday (&tv, NULL) ;
  now  = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec ;

  return (uint32_t)(now - epochMicro) ;
}


/*
 * wiringPiSetup:
 *	Must be called once at the start of your program execution.
 *
 * Default setup: Initialises the system into wiringPi Pin mode and uses the
 *	memory mapped hardware directly.
 *********************************************************************************
 */

int wiringPiSetup (void)
{
  int      fd ;
  int      boardRev ;

  if (getenv (ENV_DEBUG) != NULL)
    wiringPiDebug = TRUE ;

  if (getenv (ENV_CODES) != NULL)
    wiringPiCodes = TRUE ;

  if (geteuid () != 0)
    (void)wiringPiFailure ("wiringPiSetup: Must be root. (Did you forget sudo?)\n") ;

  if (wiringPiDebug)
    printf ("wiringPi: wiringPiSetup called\n") ;

#ifdef CB_BOARD   //this for cubieboard
	 pinToGpio = physToGpio_CB ;
	physToGpio = physToGpio_CB ;
#else  
  boardRev = piBoardRev () ;

  if (boardRev == 1)
  {
     pinToGpio =  pinToGpioR1 ;
    physToGpio = physToGpioR1 ;
  }
  else if (boardRev == 2)
  {
     pinToGpio =  pinToGpioR2 ;
    physToGpio = physToGpioR2 ;
  }
#endif

// Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0)
    (void)wiringPiFailure ("wiringPiSetup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

// GPIO:

  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);
  	if (wiringPiDebug)
 		printf("++++ gpio:0x%x\n", gpio);
  gpio += 0x21b; //for PD0 cubieboard
  	if (wiringPiDebug)
  		printf("++++ gpio PDx:0x%x\n", gpio);
  if ((int32_t)gpio == -1)
    (void)wiringPiFailure ("wiringPiSetup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

// PWM

  pwm = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_PWM) ;
  if ((int32_t)pwm == -1)
    (void)wiringPiFailure ("wiringPiSetup: mmap (PWM) failed: %s\n", strerror (errno)) ;
 
// Clock control (needed for PWM)

  clk = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, CLOCK_BASE) ;
  if ((int32_t)clk == -1)
    (void)wiringPiFailure ("wiringPiSetup: mmap (CLOCK) failed: %s\n", strerror (errno)) ;
 
// The drive pads

  pads = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_PADS) ;
  if ((int32_t)pads == -1)
    (void)wiringPiFailure ("wiringPiSetup: mmap (PADS) failed: %s\n", strerror (errno)) ;

#ifdef	USE_TIMER
// The system timer

  timer = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_TIMER) ;
  if ((int32_t)timer == -1)
    (void)wiringPiFailure ("wiringPiSetup: mmap (TIMER) failed: %s\n", strerror (errno)) ;

// Set the timer to free-running, 1MHz.
//	0xF9 is 249, the timer divide is base clock / (divide+1)
//	so base clock is 250MHz / 250 = 1MHz.

  *(timer + TIMER_CONTROL) = 0x0000280 ;
  *(timer + TIMER_PRE_DIV) = 0x00000F9 ;
  timerIrqRaw = timer + TIMER_IRQ_RAW ;
#endif

  initialiseEpoch () ;
#ifdef CB_BOARD
	wiringPiMode = WPI_MODE_PHYS ;
#else
  	wiringPiMode = WPI_MODE_PINS ;
#endif
  return 0 ;
}


/*
 * wiringPiSetupGpio:
 *	Must be called once at the start of your program execution.
 *
 * GPIO setup: Initialises the system into GPIO Pin mode and uses the
 *	memory mapped hardware directly.
 *********************************************************************************
 */

int wiringPiSetupGpio (void)
{
  (void)wiringPiSetup () ;

  if (wiringPiDebug)
    printf ("wiringPi: wiringPiSetupGpio called\n") ;
#ifdef CB_BOARD
	wiringPiMode = WPI_MODE_PHYS ;
#else
  	wiringPiMode = WPI_MODE_GPIO ;
#endif
  return 0 ;
}


/*
 * wiringPiSetupPhys:
 *	Must be called once at the start of your program execution.
 *
 * Phys setup: Initialises the system into Physical Pin mode and uses the
 *	memory mapped hardware directly.
 *********************************************************************************
 */

int wiringPiSetupPhys (void)
{
	(void)wiringPiSetup () ;

	if (wiringPiDebug)
		printf ("wiringPi: wiringPiSetupPhys called\n") ;

	wiringPiMode = WPI_MODE_PHYS ;

	return 0 ;
}


/*
 * wiringPiSetupSys:
 *	Must be called once at the start of your program execution.
 *
 * Initialisation (again), however this time we are using the /sys/class/gpio
 *	interface to the GPIO systems - slightly slower, but always usable as
 *	a non-root user, assuming the devices are already exported and setup correctly.
 */

int wiringPiSetupSys (void)
{
  int boardRev ;
  int pin ;
  char fName [128] ;

  if (getenv (ENV_DEBUG) != NULL)
    wiringPiDebug = TRUE ;

  if (getenv (ENV_CODES) != NULL)
    wiringPiCodes = TRUE ;

  if (wiringPiDebug)
    printf ("wiringPi: wiringPiSetupSys called\n") ;

#ifdef CB_BOARD
	 pinToGpio = physToGpio_CB ;
	physToGpio = physToGpio_CB ;
#else
  boardRev = piBoardRev () ;

  if (boardRev == 1)
  {
     pinToGpio =  pinToGpioR1 ;
    physToGpio = physToGpioR1 ;
  }
  else
  {
     pinToGpio =  pinToGpioR2 ;
    physToGpio = physToGpioR2 ;
  }
#endif
// Open and scan the directory, looking for exported GPIOs, and pre-open
//	the 'value' interface to speed things up for later
  
  for (pin = 1 ; pin < 5 ; ++pin) //pin may < 96
  {
    sprintf (fName, "/sys/class/gpio/gpio%d/value", pin) ; // fix this
    sysFds [pin] = open (fName, O_RDWR) ;
    printf ("open gpio%d\n", pin) ;
  }

  initialiseEpoch () ;  //what is this?
#ifdef CB_BOARD
	wiringPiMode = WPI_MODE_PHYS ;
#else  
 	wiringPiMode = WPI_MODE_GPIO_SYS ;
#endif
  return 0 ;
}
