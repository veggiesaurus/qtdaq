/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
******************************************************************************/

#ifndef _WAVEDUMP_H_
#define _WAVEDUMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <fstream.h>
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#ifdef WIN32

    #include <time.h>
    #include <sys/timeb.h>
    #include <conio.h>
    #include <process.h>

	#define getch _getch     /* redefine POSIX 'deprecated' */
	#define kbhit _kbhit     /* redefine POSIX 'deprecated' */

	#define		_PACKED_
	#define		_INLINE_		

#else
    #include <unistd.h>
    #include <stdint.h>   /* C99 compliant compilers: uint64_t */
    #include <ctype.h>    /* toupper() */
    #include <sys/time.h>

	#define		_PACKED_		__attribute__ ((packed, aligned(1)))
	#define		_INLINE_		__inline__ 

#endif

#ifdef LINUX
#define DEFAULT_CONFIG_FILE  "QtCAENConfig.txt"
#else
#define DEFAULT_CONFIG_FILE  "QtCAENConfig.txt"  /* local directory */
#endif

#define OUTFILENAME "wave"  /* The actual file name is wave_n.txt, where n is the channel */
#define MAX_CH  64          /* max. number of channels */
#define MAX_SET 8           /* max. number of independent settings */

#define MAX_GW  1000        /* max. number of generic write commads */

#define PLOT_REFRESH_TIME 1000

#define VME_INTERRUPT_LEVEL      1
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK
#define INTERRUPT_TIMEOUT        200  // ms


#endif /* _WAVEDUMP__H */
