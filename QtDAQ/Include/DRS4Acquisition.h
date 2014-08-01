#pragma once
/*
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
*/
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
#define NUM_DIGITIZER_SAMPLES 1024
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

	float custom1,custom2,custom3,custom4,custom5;
};

struct EventStatistics
{
	EventTimestamp timestamp;
	double triggerTimeAdjustedMillis;
	int serial;
	float custom1,custom2,custom3,custom4,custom5;
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