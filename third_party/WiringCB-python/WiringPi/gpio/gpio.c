/*
 * gpio.c:
 *	Swiss-Army-Knife, Set-UID command-line interface to the Raspberry
 *	Pi's GPIO.
 *	Copyright (c) 2012-2013 Gordon Henderson
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#include <wiringPi.h>

#include <gertboard.h>
#include <piFace.h>
#include <sr595.h>
#include <mcp23008.h>
#include <mcp23017.h>
#include <mcp23s08.h>
#include <mcp23s17.h>

extern int wiringPiDebug ;

#ifndef TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	VERSION	"2.00"

static int wpMode ;

char *usage = "Usage: gpio -v\n"
              "       gpio -h\n"
              "       gpio [-g|-1] [-x module:params] ...\n"
              "       gpio [-p] <read/write/wb> ...\n"
              "       gpio <read/write/aread/awritewb/pwm/clock/mode> ...\n"
	      "       gpio readall/reset\n"
	      "       gpio unexportall/exports\n"
	      "       gpio export/edge/unexport ...\n"
	      "       gpio drive <group> <value>\n"
	      "       gpio pwm-bal/pwm-ms \n"
	      "       gpio pwmr <range> \n"
	      "       gpio pwmc <divider> \n"
	      "       gpio load spi/i2c\n"
	      "       gpio gbr <channel>\n"
	      "       gpio gbw <channel> <value>" ;	// No trailing newline needed here.

struct moduleFunctionStruct
{
  const char *name ;
  int	(*function)(char *progName, int pinBase, char *params) ;
} ;

static int doModuleMcp23008 (char *progName, int pinBase, char *params)
{
  int i2c ;

// Extract the I2C address:

  if (*params != ':')
  {
    fprintf (stderr, "%s: colon expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  ++params ;
  if (!isdigit (*params))
  {
    fprintf (stderr, "%s: digit expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  i2c = strtol (params, NULL, 0) ;
  if ((i2c < 0x03) || (i2c > 0x77))
  {
    fprintf (stderr, "%s: i2c address (0x%X) out of range\n", progName, i2c) ;
    return FALSE ;
  }

  mcp23008Setup (pinBase, i2c) ;

  return TRUE ;
}

static int doModuleMcp23017 (char *progName, int pinBase, char *params)
{
  int i2c ;

// Extract the I2C address:

  if (*params != ':')
  {
    fprintf (stderr, "%s: colon expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  ++params ;
  if (!isdigit (*params))
  {
    fprintf (stderr, "%s: digit expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  i2c = strtol (params, NULL, 0) ;
  if ((i2c < 0x03) || (i2c > 0x77))
  {
    fprintf (stderr, "%s: i2c address (0x%X) out of range\n", progName, i2c) ;
    return FALSE ;
  }

  mcp23017Setup (pinBase, i2c) ;

  return TRUE ;
}

static int doModuleMcp23s08 (char *progName, int pinBase, char *params)
{
  int spi, port ;

// Extract the SPI address:

  if (*params != ':')
  {
    fprintf (stderr, "%s: colon expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  ++params ;
  if (!isdigit (*params))
  {
    fprintf (stderr, "%s: digit expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  spi = *params - '0' ;
  if ((spi < 0) || (spi > 1))
  {
    fprintf (stderr, "%s: SPI address (%d) out of range\n", progName, spi) ;
    return FALSE ;
  }

// Extract the port:

  if (*++params != ':')
  {
    fprintf (stderr, "%s: colon expected after SPI address\n", progName) ;
    return FALSE ;
  }

  ++params ;
  if (!isdigit (*params))
  {
    fprintf (stderr, "%s: digit expected after SPI address\n", progName) ;
    return FALSE ;
  }

  port = strtol (params, NULL, 0) ;
  if ((port < 0) || (port > 7))
  {
    fprintf (stderr, "%s: port address (%d) out of range\n", progName, port) ;
    return FALSE ;
  }

  mcp23s08Setup (pinBase, spi, port) ;

  return TRUE ;
}

static int doModuleMcp23s17 (char *progName, int pinBase, char *params)
{
  int spi, port ;

// Extract the SPI address:

  if (*params != ':')
  {
    fprintf (stderr, "%s: colon expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  ++params ;
  if (!isdigit (*params))
  {
    fprintf (stderr, "%s: digit expected after pin-base number\n", progName) ;
    return FALSE ;
  }

  spi = *params - '0' ;
  if ((spi < 0) || (spi > 1))
  {
    fprintf (stderr, "%s: SPI address (%d) out of range\n", progName, spi) ;
    return FALSE ;
  }

// Extract the port:

  if (*++params != ':')
  {
    fprintf (stderr, "%s: colon expected after SPI address\n", progName) ;
    return FALSE ;
  }

  ++params ;
  if (!isdigit (*params))
  {
    fprintf (stderr, "%s: digit expected after SPI address\n", progName) ;
    return FALSE ;
  }

  port = strtol (params, NULL, 0) ;
  if ((port < 0) || (port > 7))
  {
    fprintf (stderr, "%s: port address (%d) out of range\n", progName, port) ;
    return FALSE ;
  }

  mcp23s17Setup (pinBase, spi, port) ;

  return TRUE ;
}


struct moduleFunctionStruct moduleFunctions [] = 
{
  { "mcp23008",		&doModuleMcp23008 	},
  { "mcp23017",		&doModuleMcp23017 	},
  { "mcp23s08",		&doModuleMcp23s08 	},
  { "mcp23s17",		&doModuleMcp23s17 	},
  { NULL,		NULL		 	},
} ;


  


/*
 * changeOwner:
 *	Change the ownership of the file to the real userId of the calling
 *	program so we can access it.
 *********************************************************************************
 */

static void changeOwner (char *cmd, char *file)
{
  uid_t uid = getuid () ;
  uid_t gid = getgid () ;

  if (chown (file, uid, gid) != 0)
  {
    if (errno == ENOENT)	// Warn that it's not there
      fprintf (stderr, "%s: Warning: File not present: %s\n", cmd, file) ;
    else
    {
      fprintf (stderr, "%s: Unable to change ownership of %s: %s\n", cmd, file, strerror (errno)) ;
      exit (1) ;
    }
  }
}


/*
 * moduleLoaded:
 *	Return true/false if the supplied module is loaded
 *********************************************************************************
 */

static int moduleLoaded (char *modName)
{
  int len   = strlen (modName) ;
  int found = FALSE ;
  FILE *fd = fopen ("/proc/modules", "r") ;
  char line [80] ;

  if (fd == NULL)
  {
    fprintf (stderr, "gpio: Unable to check modules: %s\n", strerror (errno)) ;
    exit (1) ;
  }

  while (fgets (line, 80, fd) != NULL)
  {
    if (strncmp (line, modName, len) != 0)
      continue ;

    found = TRUE ;
    break ;
  }

  fclose (fd) ;

  return found ;
}


/*
 * doLoad:
 *	Load either the spi or i2c modules and change device ownerships, etc.
 *********************************************************************************
 */

static void _doLoadUsage (char *argv [])
{
  fprintf (stderr, "Usage: %s load <spi/i2c> [SPI bufferSize in KB | I2C baudrate in Kb/sec]\n", argv [0]) ;
  exit (1) ;
}

static void doLoad (int argc, char *argv [])
{
  char *module1, *module2 ;
  char cmd [80] ;
  char *file1, *file2 ;
  char args1 [32], args2 [32] ;

  if (argc < 3)
    _doLoadUsage (argv) ;

  args1 [0] = args2 [0] = 0 ;

  /**/ if (strcasecmp (argv [2], "spi") == 0)
  {
    module1 = "spidev" ;
    module2 = "spi_bcm2708" ;
    file1  = "/dev/spidev0.0" ;
    file2  = "/dev/spidev0.1" ;
    if (argc == 4)
      sprintf (args1, " bufsiz=%d", atoi (argv [3]) * 1024) ;
    else if (argc > 4)
      _doLoadUsage (argv) ;
  }
  else if (strcasecmp (argv [2], "i2c") == 0)
  {
    module1 = "i2c_dev" ;
    module2 = "i2c_bcm2708" ;
    file1  = "/dev/i2c-0" ;
    file2  = "/dev/i2c-1" ;
    if (argc == 4)
      sprintf (args2, " baudrate=%d", atoi (argv [3]) * 1000) ;
    else if (argc > 4)
      _doLoadUsage (argv) ;
  }
  else
    _doLoadUsage (argv) ;

  if (!moduleLoaded (module1))
  {
    sprintf (cmd, "modprobe %s%s", module1, args1) ;
    system (cmd) ;
  }

  if (!moduleLoaded (module2))
  {
    sprintf (cmd, "modprobe %s%s", module2, args2) ;
    system (cmd) ;
  }

  if (!moduleLoaded (module2))
  {
    fprintf (stderr, "%s: Unable to load %s\n", argv [0], module2) ;
    exit (1) ;
  }

  sleep (1) ;	// To let things get settled

  changeOwner (argv [0], file1) ;
  changeOwner (argv [0], file2) ;
}


/*
 * doReadall:
 *	Read all the GPIO pins
 *********************************************************************************
 */

static char *pinNames [] =
{
  "GPIO 0", "GPIO 1", "GPIO 2", "GPIO 3", "GPIO 4", "GPIO 5", "GPIO 6", "GPIO 7",
  "SDA   ", "SCL   ",
  "CE0   ", "CE1   ", "MOSI  ", "MISO  ", "SCLK  ",
  "TxD   ", "RxD   ",
  "GPIO 8", "GPIO 9", "GPIO10", "GPIO11",
} ;

static char *alts [] =
{
  "IN  ", "OUT ", "ALT5", "ALT4", "ALT0", "ALT1", "ALT2", "ALT3"
} ;

static int wpiToPhys [64] =
{
  11, 12, 13, 15, 16, 18, 22,  7,	//  0...7
   3,  5,				//  8...9
  24, 26, 19, 21, 23,			// 10..14
   8, 10,				// 15..16
   3,  4,  5,  6,			// 17..20
             0,0,0,0,0,0,0,0,0,0,0,	// 20..31
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 32..47
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 47..63
} ;

static void doReadall (void)
{
  int pin ;

  printf ("+----------+-Rev%d-+------+--------+------+-------+\n", piBoardRev ()) ;
  printf ("| wiringPi | GPIO | Phys | Name   | Mode | Value |\n") ;
  printf ("+----------+------+------+--------+------+-------+\n") ;

  for (pin = 0 ; pin < 64 ; ++pin)
  {
    if (wpiPinToGpio (pin) == -1)
      continue ;

    printf ("| %6d   | %3d  | %3d  | %s | %s | %s  |\n",
	pin, wpiPinToGpio (pin), wpiToPhys [pin],
	pinNames [pin], 
	alts [getAlt (pin)], 
	digitalRead (pin) == HIGH ? "High" : "Low ") ;
  }

  printf ("+----------+------+------+--------+------+-------+\n") ;
}


/*
 * doExports:
 *	List all GPIO exports
 *********************************************************************************
 */

static void doExports (int argc, char *argv [])
{
  int fd ;
  int i, l, first ;
  char fName [128] ;
  char buf [16] ;

// Rather crude, but who knows what others are up to...

  for (first = 0, i = 0 ; i < 64 ; ++i)
  {

// Try to read the direction

    sprintf (fName, "/sys/class/gpio/gpio%d/direction", i) ;
    if ((fd = open (fName, O_RDONLY)) == -1)
      continue ;

    if (first == 0)
    {
      ++first ;
      printf ("GPIO Pins exported:\n") ;
    }

    printf ("%4d: ", i) ;

    if ((l = read (fd, buf, 16)) == 0)
      sprintf (buf, "%s", "?") ;
 
    buf [l] = 0 ;
    if ((buf [strlen (buf) - 1]) == '\n')
      buf [strlen (buf) - 1] = 0 ;

    printf ("%-3s", buf) ;

    close (fd) ;

// Try to Read the value

    sprintf (fName, "/sys/class/gpio/gpio%d/value", i) ;
    if ((fd = open (fName, O_RDONLY)) == -1)
    {
      printf ("No Value file (huh?)\n") ;
      continue ;
    }

    if ((l = read (fd, buf, 16)) == 0)
      sprintf (buf, "%s", "?") ;

    buf [l] = 0 ;
    if ((buf [strlen (buf) - 1]) == '\n')
      buf [strlen (buf) - 1] = 0 ;

    printf ("  %s", buf) ;

// Read any edge trigger file

    sprintf (fName, "/sys/class/gpio/gpio%d/edge", i) ;
    if ((fd = open (fName, O_RDONLY)) == -1)
    {
      printf ("\n") ;
      continue ;
    }

    if ((l = read (fd, buf, 16)) == 0)
      sprintf (buf, "%s", "?") ;

    buf [l] = 0 ;
    if ((buf [strlen (buf) - 1]) == '\n')
      buf [strlen (buf) - 1] = 0 ;

    printf ("  %-8s\n", buf) ;

    close (fd) ;
  }
}


/*
 * doExport:
 *	gpio export pin mode
 *	This uses the /sys/class/gpio device interface.
 *********************************************************************************
 */

void doExport (int argc, char *argv [])
{
  FILE *fd ;
  int pin ;
  char *mode ;
  char fName [128] ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s export pin mode\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  mode = argv [3] ;

  if ((fd = fopen ("/sys/class/gpio/export", "w")) == NULL)
  {
    fprintf (stderr, "%s: Unable to open GPIO export interface: %s\n", argv [0], strerror (errno)) ;
    exit (1) ;
  }

  fprintf (fd, "%d\n", pin) ;
  fclose (fd) ;

  sprintf (fName, "/sys/class/gpio/gpio%d/direction", pin) ;
  if ((fd = fopen (fName, "w")) == NULL)
  {
    fprintf (stderr, "%s: Unable to open GPIO direction interface for pin %d: %s\n", argv [0], pin, strerror (errno)) ;
    exit (1) ;
  }

  /**/ if ((strcasecmp (mode, "in")  == 0) || (strcasecmp (mode, "input")  == 0))
    fprintf (fd, "in\n") ;
  else if ((strcasecmp (mode, "out") == 0) || (strcasecmp (mode, "output") == 0))
    fprintf (fd, "out\n") ;
  else
  {
    fprintf (stderr, "%s: Invalid mode: %s. Should be in or out\n", argv [1], mode) ;
    exit (1) ;
  }

  fclose (fd) ;

// Change ownership so the current user can actually use it!

  sprintf (fName, "/sys/class/gpio/gpio%d/value", pin) ;
  changeOwner (argv [0], fName) ;

  sprintf (fName, "/sys/class/gpio/gpio%d/edge", pin) ;
  changeOwner (argv [0], fName) ;

}


/*
 * doEdge:
 *	gpio edge pin mode
 *	Easy access to changing the edge trigger on a GPIO pin
 *	This uses the /sys/class/gpio device interface.
 *********************************************************************************
 */

void doEdge (int argc, char *argv [])
{
  FILE *fd ;
  int pin ;
  char *mode ;
  char fName [128] ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s edge pin mode\n", argv [0]) ;
    exit (1) ;
  }

  pin  = atoi (argv [2]) ;
  mode = argv [3] ;

// Export the pin and set direction to input

  if ((fd = fopen ("/sys/class/gpio/export", "w")) == NULL)
  {
    fprintf (stderr, "%s: Unable to open GPIO export interface: %s\n", argv [0], strerror (errno)) ;
    exit (1) ;
  }

  fprintf (fd, "%d\n", pin) ;
  fclose (fd) ;

  sprintf (fName, "/sys/class/gpio/gpio%d/direction", pin) ;
  if ((fd = fopen (fName, "w")) == NULL)
  {
    fprintf (stderr, "%s: Unable to open GPIO direction interface for pin %d: %s\n", argv [0], pin, strerror (errno)) ;
    exit (1) ;
  }

  fprintf (fd, "in\n") ;
  fclose (fd) ;

  sprintf (fName, "/sys/class/gpio/gpio%d/edge", pin) ;
  if ((fd = fopen (fName, "w")) == NULL)
  {
    fprintf (stderr, "%s: Unable to open GPIO edge interface for pin %d: %s\n", argv [0], pin, strerror (errno)) ;
    exit (1) ;
  }

  /**/ if (strcasecmp (mode, "none")    == 0) fprintf (fd, "none\n") ;
  else if (strcasecmp (mode, "rising")  == 0) fprintf (fd, "rising\n") ;
  else if (strcasecmp (mode, "falling") == 0) fprintf (fd, "falling\n") ;
  else if (strcasecmp (mode, "both")    == 0) fprintf (fd, "both\n") ;
  else
  {
    fprintf (stderr, "%s: Invalid mode: %s. Should be none, rising, falling or both\n", argv [1], mode) ;
    exit (1) ;
  }

// Change ownership of the value and edge files, so the current user can actually use it!

  sprintf (fName, "/sys/class/gpio/gpio%d/value", pin) ;
  changeOwner (argv [0], fName) ;

  sprintf (fName, "/sys/class/gpio/gpio%d/edge", pin) ;
  changeOwner (argv [0], fName) ;

  fclose (fd) ;
}


/*
 * doUnexport:
 *	gpio unexport pin
 *	This uses the /sys/class/gpio device interface.
 *********************************************************************************
 */

void doUnexport (int argc, char *argv [])
{
  FILE *fd ;
  int pin ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s unexport pin\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  if ((fd = fopen ("/sys/class/gpio/unexport", "w")) == NULL)
  {
    fprintf (stderr, "%s: Unable to open GPIO export interface\n", argv [0]) ;
    exit (1) ;
  }

  fprintf (fd, "%d\n", pin) ;
  fclose (fd) ;
}


/*
 * doUnexportAll:
 *	gpio unexportall
 *	Un-Export all the GPIO pins.
 *	This uses the /sys/class/gpio device interface.
 *********************************************************************************
 */

void doUnexportall (char *progName)
{
  FILE *fd ;
  int pin ;

  for (pin = 0 ; pin < 63 ; ++pin)
  {
    if ((fd = fopen ("/sys/class/gpio/unexport", "w")) == NULL)
    {
      fprintf (stderr, "%s: Unable to open GPIO export interface\n", progName) ;
      exit (1) ;
    }
    fprintf (fd, "%d\n", pin) ;
    fclose (fd) ;
  }
}


/*
 * doReset:
 *	Reset the GPIO pins - as much as we can do
 *********************************************************************************
 */

static void doReset (char *progName)
{
  int pin ;

  doUnexportall (progName) ;

  for (pin = 0 ; pin < 64 ; ++pin)
  {
    if (wpiPinToGpio (pin) == -1)
      continue ;

    digitalWrite    (pin, LOW) ;
    pinMode         (pin, INPUT) ;
    pullUpDnControl (pin, PUD_OFF) ;
  }
}


/*
 * doMode:
 *	gpio mode pin mode ...
 *********************************************************************************
 */

void doMode (int argc, char *argv [])
{
  int pin ;
  char *mode ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s mode pin mode\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  mode = argv [3] ;

  /**/ if (strcasecmp (mode, "in")     == 0) pinMode         (pin, INPUT) ;
  else if (strcasecmp (mode, "out")    == 0) pinMode         (pin, OUTPUT) ;
  else if (strcasecmp (mode, "pwm")    == 0) pinMode         (pin, PWM_OUTPUT) ;
  else if (strcasecmp (mode, "clock")  == 0) pinMode         (pin, GPIO_CLOCK) ;
  else if (strcasecmp (mode, "up")     == 0) pullUpDnControl (pin, PUD_UP) ;
  else if (strcasecmp (mode, "down")   == 0) pullUpDnControl (pin, PUD_DOWN) ;
  else if (strcasecmp (mode, "tri")    == 0) pullUpDnControl (pin, PUD_OFF) ;
  else
  {
    fprintf (stderr, "%s: Invalid mode: %s. Should be in/out/pwm/clock/up/down/tri\n", argv [1], mode) ;
    exit (1) ;
  }
}


/*
 * doPadDrive:
 *	gpio drive group value
 *********************************************************************************
 */

static void doPadDrive (int argc, char *argv [])
{
  int group, val ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s drive group value\n", argv [0]) ;
    exit (1) ;
  }

  group = atoi (argv [2]) ;
  val   = atoi (argv [3]) ;

  if ((group < 0) || (group > 2))
  {
    fprintf (stderr, "%s: drive group not 0, 1 or 2: %d\n", argv [0], group) ;
    exit (1) ;
  }

  if ((val < 0) || (val > 7))
  {
    fprintf (stderr, "%s: drive value not 0-7: %d\n", argv [0], val) ;
    exit (1) ;
  }

  setPadDrive (group, val) ;
}


/*
 * doGbw:
 *	gpio gbw channel value
 *	Gertboard Write - To the Analog output
 *********************************************************************************
 */

static void doGbw (int argc, char *argv [])
{
  int channel, value ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s gbr <channel> <value>\n", argv [0]) ;
    exit (1) ;
  }

  channel = atoi (argv [2]) ;
  value   = atoi (argv [3]) ;

  if ((channel < 0) || (channel > 1))
  {
    fprintf (stderr, "%s: channel must be 0 or 1\n", argv [0]) ;
    exit (1) ;
  }

  if ((value < 0) || (value > 1023))
  {
    fprintf (stderr, "%s: value must be from 0 to 255\n", argv [0]) ;
    exit (1) ;
  }

  if (gertboardAnalogSetup (64) < 0)
  {
    fprintf (stderr, "Unable to initialise the Gertboard SPI interface: %s\n", strerror (errno)) ;
    exit (1) ;
  }

  analogWrite (64 + channel, value) ;
}


/*
 * doGbr:
 *	gpio gbr channel
 *	From the analog input
 *********************************************************************************
 */

static void doGbr (int argc, char *argv [])
{
  int channel ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s gbr <channel>\n", argv [0]) ;
    exit (1) ;
  }

  channel = atoi (argv [2]) ;

  if ((channel < 0) || (channel > 1))
  {
    fprintf (stderr, "%s: channel must be 0 or 1\n", argv [0]) ;
    exit (1) ;
  }

  if (gertboardAnalogSetup (64) < 0)
  {
    fprintf (stderr, "Unable to initialise the Gertboard SPI interface: %s\n", strerror (errno)) ;
    exit (1) ;
  }

  printf ("%d\n", analogRead (64 + channel)) ;
}


/*
 * doWrite:
 *	gpio write pin value
 *********************************************************************************
 */

static void doWrite (int argc, char *argv [])
{
  int pin, val ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s write pin value\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  /**/ if ((strcasecmp (argv [3], "up") == 0) || (strcasecmp (argv [3], "on") == 0))
    val = 1 ;
  else if ((strcasecmp (argv [3], "down") == 0) || (strcasecmp (argv [3], "off") == 0))
    val = 0 ;
  else
    val = atoi (argv [3]) ;

  /**/ if (val == 0)
    digitalWrite (pin, LOW) ;
  else
    digitalWrite (pin, HIGH) ;
}


/*
 * doAwriterite:
 *	gpio awrite pin value
 *********************************************************************************
 */

static void doAwrite (int argc, char *argv [])
{
  int pin, val ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s awrite pin value\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  val = atoi (argv [3]) ;

  analogWrite (pin, val) ;
}


/*
 * doWriteByte:
 *	gpio write value
 *********************************************************************************
 */

static void doWriteByte (int argc, char *argv [])
{
  int val ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s wb value\n", argv [0]) ;
    exit (1) ;
  }

  val = (int)strtol (argv [2], NULL, 0) ;

  digitalWriteByte (val) ;
}


/*
 * doRead:
 *	Read a pin and return the value
 *********************************************************************************
 */

void doRead (int argc, char *argv []) 
{
  int pin, val ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s read pin\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  val = digitalRead (pin) ;

  printf ("%s\n", val == 0 ? "0" : "1") ;
}


/*
 * doAread:
 *	Read an analog pin and return the value
 *********************************************************************************
 */

void doAread (int argc, char *argv []) 
{
  int pin, val ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s aread pin\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  val = analogRead (pin) ;

  printf ("%s\n", val == 0 ? "0" : "1") ;
}


/*
 * doClock:
 *	Output a clock on a pin
 *********************************************************************************
 */

void doClock (int argc, char *argv [])
{
  int pin, freq ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s clock <pin> <freq>\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  freq = atoi (argv [3]) ;

  gpioClockSet (pin, freq) ;
}


/*
 * doPwm:
 *	Output a PWM value on a pin
 *********************************************************************************
 */

void doPwm (int argc, char *argv [])
{
  int pin, val ;

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s pwm <pin> <value>\n", argv [0]) ;
    exit (1) ;
  }

  pin = atoi (argv [2]) ;

  val = atoi (argv [3]) ;

  pwmWrite (pin, val) ;
}


/*
 * doPwmMode: doPwmRange: doPwmClock:
 *	Change the PWM mode, range and clock divider values
 *********************************************************************************
 */

static void doPwmMode (int mode)
{
  pwmSetMode (mode) ;
}

static void doPwmRange (int argc, char *argv [])
{
  unsigned int range ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s pwmr <range>\n", argv [0]) ;
    exit (1) ;
  }

  range = (unsigned int)strtoul (argv [2], NULL, 10) ;

  if (range == 0)
  {
    fprintf (stderr, "%s: range must be > 0\n", argv [0]) ;
    exit (1) ;
  }

  pwmSetRange (range) ;
}

static void doPwmClock (int argc, char *argv [])
{
  unsigned int clock ;

  if (argc != 3)
  {
    fprintf (stderr, "Usage: %s pwmc <clock>\n", argv [0]) ;
    exit (1) ;
  }

  clock = (unsigned int)strtoul (argv [2], NULL, 10) ;

  if ((clock < 1) || (clock > 4095))
  {
    fprintf (stderr, "%s: clock must be between 0 and 4096\n", argv [0]) ;
    exit (1) ;
  }

  pwmSetClock (clock) ;
}


/*
 * doModule:
 *	Load in a wiringPi extension module
 *********************************************************************************
 */

static int doModule (char *progName, char *moduleData)
{
  char *p ;
  char *module = moduleData ;
  struct moduleFunctionStruct *modFn ;
  int pinBase = 0 ;

// Get the module name by finding the first :

  p = module ;
  while (*p != ':')
  {
    if (!*p)	// ran out of characters
    {
      fprintf (stderr, "%s: module name not terminated by a colon\n", progName) ;
      return FALSE ;
    }
    ++p ;
  }

  *p++ = 0 ;

  if (!isdigit (*p))
  {
    fprintf (stderr, "%s: pinBase number expected after module name\n", progName) ;
    return FALSE ;
  }

  while (isdigit (*p))
  {
    if (pinBase > 1000000000)
    {
      fprintf (stderr, "%s: pinBase too large\n", progName) ;
      return FALSE ;
    }

    pinBase = pinBase * 10 + (*p - '0') ;
    ++p ;
  }

  if (pinBase < 64)
  {
    fprintf (stderr, "%s: pinBase (%d) too small. Minimum is 64.\n", progName, pinBase) ;
    return FALSE ;
  }

// Search for modules:

  for (modFn = moduleFunctions ; modFn->name != NULL ; ++modFn)
  {
    if (strcmp (modFn->name, module) == 0)
      return modFn->function (progName, pinBase, p) ;
  }

  fprintf (stderr, "%s: module %s not found\n", progName, module) ;
  return FALSE ;
}



/*
 * main:
 *	Start here
 *********************************************************************************
 */

int main (int argc, char *argv [])
{
  int i ;

  if (getenv ("WIRINGPI_DEBUG") != NULL)
  {
    printf ("gpio: wiringPi debug mode enabled\n") ;
    wiringPiDebug = TRUE ;
  }

  if (argc == 1)
  {
    fprintf (stderr, "%s\n", usage) ;
    return 1 ;
  }

  if (strcasecmp (argv [1], "-h") == 0)
  {
    printf ("%s: %s\n", argv [0], usage) ;
    return 0 ;
  }

  if (strcasecmp (argv [1], "-v") == 0)
  {
    printf ("gpio version: %s\n", VERSION) ;
    printf ("Copyright (c) 2012-2013 Gordon Henderson\n") ;
    printf ("This is free software with ABSOLUTELY NO WARRANTY.\n") ;
    printf ("For details type: %s -warranty\n", argv [0]) ;
    printf ("\n") ;
    printf ("This Raspberry Pi is a revision %d board.\n", piBoardRev ()) ;
    return 0 ;
  }

  if (strcasecmp (argv [1], "-warranty") == 0)
  {
    printf ("gpio version: %s\n", VERSION) ;
    printf ("Copyright (c) 2012-2013 Gordon Henderson\n") ;
    printf ("\n") ;
    printf ("    This program is free software; you can redistribute it and/or modify\n") ;
    printf ("    it under the terms of the GNU Leser General Public License as published\n") ;
    printf ("    by the Free Software Foundation, either version 3 of the License, or\n") ;
    printf ("    (at your option) any later version.\n") ;
    printf ("\n") ;
    printf ("    This program is distributed in the hope that it will be useful,\n") ;
    printf ("    but WITHOUT ANY WARRANTY; without even the implied warranty of\n") ;
    printf ("    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n") ;
    printf ("    GNU Lesser General Public License for more details.\n") ;
    printf ("\n") ;
    printf ("    You should have received a copy of the GNU Lesser General Public License\n") ;
    printf ("    along with this program. If not, see <http://www.gnu.org/licenses/>.\n") ;
    printf ("\n") ;
    return 0 ;
  }

  if (geteuid () != 0)
  {
    fprintf (stderr, "%s: Must be root to run. Program should be suid root. This is an error.\n", argv [0]) ;
    return 1 ;
  }

// Initial test for /sys/class/gpio operations:

  /**/ if (strcasecmp (argv [1], "exports"    ) == 0)	{ doExports     (argc, argv) ;	return 0 ; }
  else if (strcasecmp (argv [1], "export"     ) == 0)	{ doExport      (argc, argv) ;	return 0 ; }
  else if (strcasecmp (argv [1], "edge"       ) == 0)	{ doEdge        (argc, argv) ;	return 0 ; }
  else if (strcasecmp (argv [1], "unexport"   ) == 0)	{ doUnexport    (argc, argv) ;	return 0 ; }
  else if (strcasecmp (argv [1], "unexportall") == 0)	{ doUnexportall (argv [0]) ;	return 0 ; }

// Check for load command:

  if (strcasecmp (argv [1], "load" ) == 0)	{ doLoad     (argc, argv) ; return 0 ; }

// Gertboard commands

  if (strcasecmp (argv [1], "gbr" ) == 0)	{ doGbr (argc, argv) ; return 0 ; }
  if (strcasecmp (argv [1], "gbw" ) == 0)	{ doGbw (argc, argv) ; return 0 ; }

// Check for -g argument

  /**/ if (strcasecmp (argv [1], "-g") == 0)
  {
    wiringPiSetupGpio () ;

    for (i = 2 ; i < argc ; ++i)
      argv [i - 1] = argv [i] ;
    --argc ;
    wpMode = WPI_MODE_GPIO ;
  }

// Check for -1 argument

  else if (strcasecmp (argv [1], "-1") == 0)
  {
    wiringPiSetupPhys () ;

    for (i = 2 ; i < argc ; ++i)
      argv [i - 1] = argv [i] ;
    --argc ;
    wpMode = WPI_MODE_PHYS ;
  }

// Check for -p argument for PiFace

  else if (strcasecmp (argv [1], "-p") == 0)
  {
    piFaceSetup (200) ;

    for (i = 2 ; i < argc ; ++i)
      argv [i - 1] = argv [i] ;
    --argc ;
    wpMode = WPI_MODE_PIFACE ;
  }

// Default to wiringPi mode

  else
  {
    wiringPiSetup () ;
    wpMode = WPI_MODE_PINS ;
  }

// Check for -x argument to load in a new module

  if (strcasecmp (argv [1], "-x") == 0)
  {
    if (argc < 3)
    {
      fprintf (stderr, "%s: -x missing module specification.\n", argv [0]) ;
      exit (EXIT_FAILURE) ;
    }

    if (!doModule (argv [0], argv [2]))	// Prints its own error messages
      exit (EXIT_FAILURE) ;

    for (i = 3 ; i < argc ; ++i)
      argv [i - 2] = argv [i] ;
    argc -= 2 ;
  }

  if (argc <= 1)
  {
    fprintf (stderr, "%s: no command given\n", argv [0]) ;
    exit (EXIT_FAILURE) ;
  }

// Core wiringPi functions

  /**/ if (strcasecmp (argv [1], "mode"   ) == 0) doMode      (argc, argv) ;
  else if (strcasecmp (argv [1], "read"   ) == 0) doRead      (argc, argv) ;
  else if (strcasecmp (argv [1], "write"  ) == 0) doWrite     (argc, argv) ;
  else if (strcasecmp (argv [1], "pwm"    ) == 0) doPwm       (argc, argv) ;
  else if (strcasecmp (argv [1], "awrite" ) == 0) doAwrite    (argc, argv) ;
  else if (strcasecmp (argv [1], "aread"  ) == 0) doAread     (argc, argv) ;

// Pi Specifics

  else if (strcasecmp (argv [1], "pwm-bal") == 0) doPwmMode   (PWM_MODE_BAL) ;
  else if (strcasecmp (argv [1], "pwm-ms" ) == 0) doPwmMode   (PWM_MODE_MS) ;
  else if (strcasecmp (argv [1], "pwmr"   ) == 0) doPwmRange  (argc, argv) ;
  else if (strcasecmp (argv [1], "pwmc"   ) == 0) doPwmClock  (argc, argv) ;
  else if (strcasecmp (argv [1], "drive"  ) == 0) doPadDrive  (argc, argv) ;
  else if (strcasecmp (argv [1], "readall") == 0) doReadall   () ;
  else if (strcasecmp (argv [1], "reset"  ) == 0) doReset     (argv [0]) ;
  else if (strcasecmp (argv [1], "wb"     ) == 0) doWriteByte (argc, argv) ;
  else if (strcasecmp (argv [1], "clock"  ) == 0) doClock     (argc, argv) ;
  else
  {
    fprintf (stderr, "%s: Unknown command: %s.\n", argv [0], argv [1]) ;
    exit (EXIT_FAILURE) ;
  }
  return 0 ;
}
