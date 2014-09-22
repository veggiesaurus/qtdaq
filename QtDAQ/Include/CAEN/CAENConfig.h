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
#pragma once
#include "WaveDump.h"
#include <qtextstream.h>

#define SAFE_DELETE( ptr ) \
if (ptr != NULL)      \
{                     \
    delete ptr;       \
    ptr = NULL;       \
}

#define SAFE_DELETE_ARRAY( ptr ) \
if (ptr != NULL)            \
{                           \
    delete[] ptr;           \
    ptr = NULL;             \
}

#ifdef WIN32
#define TIME_T __time64_t
#define TIME _time64
#define DIFFTIME _difftime64
#else
#define TIME_T time_t
#define TIME time
#define DIFFTIME difftime

#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define INT8 int8_t
#define INT16 int16_t
#define INT32 int32_t

#define MAXUINT8    ((UINT8)~((UINT8)0))
#define MAXINT8     ((INT8)(MAXUINT8 >> 1))
#define MININT8     ((INT8)~MAXINT8)

#define MAXUINT16   ((UINT16)~((UINT16)0))
#define MAXINT16    ((INT16)(MAXUINT16 >> 1))
#define MININT16    ((INT16)~MAXINT16)

#define MAXUINT32   ((UINT32)~((UINT32)0))
#define MAXINT32    ((INT32)(MAXUINT32 >> 1))
#define MININT32    ((INT32)~MAXINT32)

#define MAXUINT64   ((UINT64)~((UINT64)0))
#define MAXINT64    ((INT64)(MAXUINT64 >> 1))
#define MININT64    ((INT64)~MAXINT64)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif

/* ###########################################################################
   Typedefs
   ###########################################################################
*/

typedef enum {
	BinaryOutput=	0x00000001,			// Bit 0: 1 = BINARY, 0 =ASCII
	HeaderOutput= 0x00000002,			// Bit 1: 1 = include header, 0 = just samples data
} OutputFlags;

inline OutputFlags operator|(OutputFlags a, OutputFlags b)
{return static_cast<OutputFlags>(static_cast<int>(a) | static_cast<int>(b));}

struct CAENConfig 
{
	//link type (optical, usb, vme)
    CAEN_DGTZ_ConnectionType LinkType;	
    int LinkNum;
    int ConetNode;
	//The base address of the digitizer
    uint32_t BaseAddress;
    int NumChannels;
    int NumBits;
    float Ts;
    int NumEvents;
    int RecordLength;
    int PostTrigger;
    int InterruptNumEvents;
    int TestPattern;
    CAEN_DGTZ_EnaDis_t DesMode;
    CAEN_DGTZ_TriggerPolarity_t TriggerEdge;
    CAEN_DGTZ_IOLevel_t FPIOtype;
    CAEN_DGTZ_TriggerMode_t ExtTriggerMode;
    uint8_t EnableMask;
    CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[MAX_SET];
    uint32_t DCoffset[MAX_SET];
    int32_t  DCoffsetGrpCh[MAX_SET][MAX_SET];
    uint32_t Threshold[MAX_SET];
	uint8_t GroupTrgEnableMask[MAX_SET];
	
	uint32_t FTDCoffset[MAX_SET];
	uint32_t FTThreshold[MAX_SET];
	CAEN_DGTZ_TriggerMode_t	FastTriggerMode;
	CAEN_DGTZ_EnaDis_t	 FastTriggerEnabled;
	//Number of Generic write commands
    int GWn;
	//Generic write addresses
    uint32_t GWaddr[MAX_GW];
	//Generic write data
    uint32_t GWdata[MAX_GW];
	OutputFlags OutFileFlags;

    int useCorrections;

    //QtCAEN settings
	//channelEnabledFlags
    uint32_t ChMask;
    //number of samples to use for baseline calculation
    int NumBaselineSamples;
    //cutoff voltage: Below this, saturation has occurred and data should be chucked out
    uint16_t PeakMinAdcCount;

    int SamplesPerMicrosecond;
    //alpha factor for low pass filter (divided by 100 when run)
    int LowPassAlpha;

    // minimum number of consecutive samples above/below trigger threshold
    int TriggerMinLength;
    //threshold for delta-based trigger
    uint16_t DeltaTriggerThreshold;
    //threshold for delta-based trigger on second peak
    uint16_t DeltaTriggerThresholdSecondPeak;
    //number of samples AFTER trigger to look for maximum
    int TriggerSearchRange;
    //GATE offsets: The gate offsets in samples are calculated as OFFSET_NS * SAMPLES_PER_NANOSECOND
    //start gate offset in nanoseconds
    int IntegralStartOffset;
    //short gate offset in nanoseconds
    int IntegralShortGateOffset;
    //long gate offset in nanoseconds
    int IntegralLongGateOffset;
    //gain
    int GainMultiplier;
    //other settings
    //number of cached events to store for examining (NOT USED)
    int NumCachedEvents;
    //display signal: 0 - no display; 1 - display double peak events; 2 - display all events
    int DisplaySignal;
    //time (in ms) to sleep thread after displaying signal
    int DisplaySignalDelay;

	CAENConfig();
};

/* Error messages */
enum CAENErrorCode {
    ERR_NONE= 0,
    ERR_CONF_FILE_NOT_FOUND,
	ERR_CONF_FILE_INVALID,
    ERR_DGZ_OPEN,
    ERR_BOARD_INFO_READ,
    ERR_INVALID_BOARD_TYPE,
    ERR_DGZ_PROGRAM,
    ERR_MALLOC,
    ERR_RESTART,
    ERR_INTERRUPT,
    ERR_READOUT,
    ERR_EVENT_BUILD,
    ERR_HISTO_MALLOC,
    ERR_UNHANDLED_BOARD,
    ERR_OUTFILE_WRITE,
	ERR_UNKNOWN,
    ERR_DUMMY_LAST,
};

enum CAENStatus
{
	STATUS_CLOSED=0,
	STATUS_INITIALISED=1,
	STATUS_PROGRAMMED=2,
	STATUS_RUNNING=4,
	STATUS_READY=8,
	STATUS_ERROR=16,
};

inline CAENStatus operator|(CAENStatus a, CAENStatus b)
{return static_cast<CAENStatus>(static_cast<int>(a) | static_cast<int>(b));}

inline CAENStatus operator |=(CAENStatus& a, const CAENStatus& b)
{a= static_cast<CAENStatus>(static_cast<int>(a) | static_cast<int>(b));return a;}

inline CAENStatus operator&(CAENStatus a, CAENStatus b)
{return static_cast<CAENStatus>(static_cast<int>(a) & static_cast<int>(b));}

inline CAENStatus operator &=(CAENStatus& a, const CAENStatus& b)
{a= static_cast<CAENStatus>(static_cast<int>(a) & static_cast<int>(b));return a;}

inline CAENStatus operator ~(const CAENStatus& a)
{return static_cast<CAENStatus>(~static_cast<int>(a));}



static char ErrMsg[ERR_DUMMY_LAST][100] = {
    "No Error",                                         /* ERR_NONE */
    "Configuration File not found",                     /* ERR_CONF_FILE_NOT_FOUND */
    "Configuration File is invalid",                         /* ERR_CONF_FILE_INVALID */
    "Can't open the digitizer",                         /* ERR_DGZ_OPEN */
    "Can't read the Board Info",                        /* ERR_BOARD_INFO_READ */
    "Can't run WaveDump for this digitizer",            /* ERR_INVALID_BOARD_TYPE */
    "Can't program the digitizer",                      /* ERR_DGZ_PROGRAM */
    "Can't allocate the memory for the readout buffer", /* ERR_MALLOC */
    "Restarting Error",                                 /* ERR_RESTART */
    "Interrupt Error",                                  /* ERR_INTERRUPT */
    "Readout Error",                                    /* ERR_READOUT */
    "Event Build Error",                                /* ERR_EVENT_BUILD */
    "Can't allocate the memory fro the histograms",     /* ERR_HISTO_MALLOC */
    "Unhandled board type",                             /* ERR_UNHANDLED_BOARD */
    "Output file write error",                          /* ERR_OUTFILE_WRITE */
    "Uknown error",										/* ERR_UNKNOWN */

};


/* Function prototypes */
/*! \fn		ParseConfigFile(char* fileName, CAENConfig *caenCfg, QTextStream& textStream);
*   \brief   Read the configuration file and set the WaveDump paremeters
*            
*   \param   fileName        input file name
*   \param   caenCfg:   Pointer to the WaveDumpConfig data structure
*   \param   textStream: Pointer to the text stream to log to
*   \return  CAENErrorCode: ERR_NONE if success, ERR_CONF_INVALID if error
*/
CAENErrorCode ParseConfigFile(const char* fileName, CAENConfig *caenCfg, QTextStream& textStream);
