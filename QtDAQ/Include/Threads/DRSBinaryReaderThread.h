#pragma once

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include <QtSerialPort\qserialport>
#include "DRS4Acquisition.h"
#include "ProcessRoutines.h"
#include "AnalysisConfig.h"
#include "vector/vectorclass.h"
#include "globals.h"

//"EHDR" chars -> 1380206661 int
#define MAGIC_NUMBER_EVENT_HEADER 1380206661
//"C001" chars -> 825241667 int
#define MAGIC_NUMBER_CHANNEL0_HEADER 825241667
#define MAGIC_NUMBER_CHANNEL_DIFF 16777216
//buffering
#define NUM_BUFFERED_EVENTS 8

class DRSBinaryReaderThread : public QThread
{
	 Q_OBJECT
public:
	 DRSBinaryReaderThread(QObject *parent = 0);
	 ~DRSBinaryReaderThread();
	 bool initDRSBinaryReaderThread(QString filename, bool isCompressedInput, int s_runIndex, AnalysisConfig* s_analysisConfig, int updateTime=125);
	void processEvent(EventRawData rawEvent, bool outputSample);
 signals:
	 void newProcessedEvents(QVector<EventStatistics*>*);
	 void newEventSample(EventSampleData*);
	 void eventReadingFinished();

public slots:
	void stopReading(bool forceExit);
	void rewindFile();
	void onUpdateTimerTimeout();
	void onTemperatureUpdated(float temp);
	void setPaused(bool paused);
private:
    void run();

private:
	int eventIndex;
	EventRawData rawData[NUM_BUFFERED_EVENTS];
	int eventSize=0;
	char* buffer=nullptr;
	float* tempValArray;
	float* tempFilteredValArray;
	bool channelEnabled[NUM_DIGITIZER_CHANNELS];
    QVector<EventStatistics*>* processedEvents;
	FILE* inputFile=nullptr;
	bool compressedInput;
	int numEventsInFile;
	int numEventsRead;
	EventTimestamp firstEventTimestamp;
	QTimer updateTimer;
	bool sampleNextEvent=true;
	AnalysisConfig* analysisConfig;
	//temperature sensing
	float currentTemp=-1;
	bool requiresPause;
	bool ungracefulReadExit = false;

	//Mutexes
	QMutex processedEventsMutex;
	QMutex fileMutex;
	QMutex pauseMutex;
	//doesn't need to be atomic in this case, because replaying is done by destroying and re-creating threads
	int runIndex;

};

