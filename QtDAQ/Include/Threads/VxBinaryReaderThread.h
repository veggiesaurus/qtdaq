#pragma once

#include <basetsd.h>
#include <time.h>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QFile>
#include <QMutex>
#include "DRS4Acquisition.h"
#include "AnalysisConfig.h"
#include "zlib/zlib.h"

#define MAX_UINT16_CHANNEL_SIZE				64
#define EVENT_BUFFER_SIZE	1024

#define MAX_X742_CHANNEL_SIZE				9
#define MAX_X742_GROUP_SIZE					4

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



typedef struct 
{
	uint32_t			 EventSize;
	uint32_t			 BoardId;
	uint32_t			 Pattern;
	uint32_t			 ChannelMask;
	uint32_t			 EventCounter;
	uint32_t			 TriggerTimeTag;
} CAEN_DGTZ_EventInfo_t;



typedef struct 
{
	uint32_t			ChSize[MAX_UINT16_CHANNEL_SIZE]; // the number of samples stored in DataChannel array  
	uint16_t			*DataChannel[MAX_UINT16_CHANNEL_SIZE]; // the array of ChSize samples
} CAEN_DGTZ_UINT16_EVENT_t;

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
	CAEN_DGTZ_UINT16_EVENT_t data;
	CAEN_DGTZ_FLOAT_EVENT_t fData;
	static EventVx* eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_UINT16_EVENT_t* data);
	static EventVx* eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_FLOAT_EVENT_t* fData);
};

void freeEvent(EventVx* &ev);
void freeEvent(EventVx &ev);

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

class VxBinaryReaderThread : public QThread
{
	 Q_OBJECT
public:
	 VxBinaryReaderThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, QObject *parent = 0);
	 bool initVxBinaryReaderThread(QString filename, bool isCompressedInput, int updateTime=100);
	bool isReading();
 signals:
	 void newRawEvents(QVector<EventVx*>*);
	 void eventReadingFinished();

private:
    void run();
 
public slots:
	void stopReading(bool forceExit);
	void rewindFile();
	void onUpdateTimerTimeout();
	void setPaused(bool paused);
	
private:
	int eventIndex;
	DataHeader header;

	gzFile inputFileCompressed;
	QString filename;
	int numEventsInFile;
	int numEventsRead;
	QTimer* updateTimer;
	bool doRewindFile;
	bool doExitReadLoop;
	bool isReadingFile;
	AnalysisConfig* analysisConfig;
	bool requiresPause;
	QMutex pauseMutex;


	QMutex* rawMutexes[2];
	EventVx* rawBuffers[2];
	int currentBufferIndex;
	int currentBufferPosition;

	//vx1742 switches
	bool vx1742Mode;	
};