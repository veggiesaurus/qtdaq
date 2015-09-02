#pragma once

#include <basetsd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <CAENDigitizer.h>

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


#define NUM_DIGITIZER_CHANNELS 4
#define NUM_DIGITIZER_SAMPLES_DRS 1024

#define EVENT_BUFFER_SIZE	128

#ifdef WIN32
#ifndef int8_t
#define int8_t  INT8
#endif
#ifndef int16_t
#define int16_t INT16
#endif
#ifndef int32_t
#define int32_t INT32
#endif
#ifndef int64_t
#define int64_t INT64
#endif
#ifndef uint8_t
#define uint8_t  UINT8
#endif
#ifndef uint16_t
#define uint16_t UINT16
#endif
#ifndef uint32_t
#define uint32_t UINT32
#endif
#ifndef uint64_t
#define uint64_t UINT64
#endif
#endif

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


#pragma region enums
enum CAENStatus
{
	STATUS_CLOSED = 0,
	STATUS_INITIALISED = 1,
	STATUS_PROGRAMMED = 2,
	STATUS_RUNNING = 4,
	STATUS_READY = 8,
	STATUS_ERROR = 16,
};

inline CAENStatus operator|(CAENStatus a, CAENStatus b)
{
	return static_cast<CAENStatus>(static_cast<int>(a) | static_cast<int>(b));
}

inline CAENStatus operator |=(CAENStatus& a, const CAENStatus& b)
{
	a = static_cast<CAENStatus>(static_cast<int>(a) | static_cast<int>(b)); return a;
}

inline CAENStatus operator&(CAENStatus a, CAENStatus b)
{
	return static_cast<CAENStatus>(static_cast<int>(a)& static_cast<int>(b));
}

inline CAENStatus operator &=(CAENStatus& a, const CAENStatus& b)
{
	a = static_cast<CAENStatus>(static_cast<int>(a)& static_cast<int>(b)); return a;
}

inline CAENStatus operator ~(const CAENStatus& a)
{
	return static_cast<CAENStatus>(~static_cast<int>(a));
}

inline CAEN_DGTZ_ErrorCode operator|(CAEN_DGTZ_ErrorCode a, CAEN_DGTZ_ErrorCode b)
{
	return static_cast<CAEN_DGTZ_ErrorCode>(static_cast<int>(a) | static_cast<int>(b));
}

inline CAEN_DGTZ_ErrorCode operator |=(CAEN_DGTZ_ErrorCode& a, const CAEN_DGTZ_ErrorCode& b)
{
	a = static_cast<CAEN_DGTZ_ErrorCode>(static_cast<int>(a) | static_cast<int>(b)); return a;
}

inline CAEN_DGTZ_ErrorCode operator&(CAEN_DGTZ_ErrorCode a, CAEN_DGTZ_ErrorCode b)
{
	return static_cast<CAEN_DGTZ_ErrorCode>(static_cast<int>(a)& static_cast<int>(b));
}

inline CAEN_DGTZ_ErrorCode operator &=(CAEN_DGTZ_ErrorCode& a, const CAEN_DGTZ_ErrorCode& b)
{
	a = static_cast<CAEN_DGTZ_ErrorCode>(static_cast<int>(a)& static_cast<int>(b)); return a;
}

inline CAEN_DGTZ_ErrorCode operator ~(const CAEN_DGTZ_ErrorCode& a)
{
	return static_cast<CAEN_DGTZ_ErrorCode>(~static_cast<int>(a));
}

enum CAENErrorCode {
	ERR_NONE = 0,
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


#pragma endregion 


typedef struct
{
	uint32_t			ChSize[MAX_UINT16_CHANNEL_SIZE]; // the number of samples stored in DataChannel array  
	float			*DataChannel[MAX_UINT16_CHANNEL_SIZE]; // the array of ChSize samples
} CAEN_DGTZ_FLOAT_EVENT_t;

//structure to hold a general 16-bit event
struct EventVx
{
	CAEN_DGTZ_EventInfo_t info;
	bool processed;
	int runIndex;
	CAEN_DGTZ_UINT16_EVENT_t data;
	CAEN_DGTZ_FLOAT_EVENT_t fData;
	static EventVx* eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_UINT16_EVENT_t* data);
	static EventVx* eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_FLOAT_EVENT_t* fData);
	void loadFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_UINT16_EVENT_t* data);
};

void freeVxEvent(EventVx* &ev);
void freeVxEvent(EventVx &ev);

//structure to hold information about aquisition of data
struct DataHeader
{
	//Magic number (with version)
	char magicNumber[8];
	//data description
	char description[32];
	//number of events in the file
	uint32_t numEvents;
	//the DC offset (in units of ADC counts)
	uint16_t offsetDC;
	//number of samples per event
	uint16_t numSamples;
	//million samples per second
	uint16_t MSPS;
	//adc count of saturation voltage (below which events are discarded)
	uint16_t peakSaturation;
	//date and time of aquisition start
	TIME_T dateTime;
	//reserved for future use
	char futureUse[4];
	DataHeader();
};

struct EventTimestamp
{
	unsigned short year, month, day, hour, minute, second, millisecond;
	unsigned short reserved;
};


struct SampleStatistics
{
	unsigned int channelNumber;
	float baseline;

	float minValue;
	float timeOfMin;
	int indexOfMin;

	float maxValue;
	float timeOfMax;
	int indexOfMax;

	float halfRiseValue;
	float timeOfHalfRise;
	int indexOfHalfRise;

	float timeOfCFDCrossing;
	float deltaTprevChannelCFD;

	float shortGateIntegral;
	float longGateIntegral;
	float PSD;
	float filteredPSD;

	float temperature;
	float secondsFromFirstEvent;
	float timeOfFlight;

	float custom1, custom2, custom3, custom4, custom5;
};

struct EventStatistics
{
	EventTimestamp timestamp;
	double triggerTimeAdjustedMillis;
	int serial;
	int runIndex;
	bool isValid;
	float custom1, custom2, custom3, custom4, custom5;
	SampleStatistics channelStatistics[NUM_DIGITIZER_CHANNELS];
};

struct EventRawData
{
	EventTimestamp* timestamp;
	int* serial;
	float* tValues;
	unsigned short* fValues[NUM_DIGITIZER_CHANNELS];
	void* additional;
};

struct EventSampleData
{
	EventStatistics stats;
	int numSamples;
	int MSPS;
	float* tValues;
	float* fValues[NUM_DIGITIZER_CHANNELS];
	float baseline;
	float cfdTime;
	int indexStart, indexShortEnd, indexLongEnd;
	int indexOfTOFStartPulse, indexOfTOFEndPulse;
};