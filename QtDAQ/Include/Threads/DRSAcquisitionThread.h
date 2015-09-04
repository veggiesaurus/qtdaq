#pragma once

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QVector>

#include "DRS4/DRS.h"
#include "AcquisitionDefinitions.h"
#include "DRSAcquisitionConfig.h"
#include "AnalysisConfig.h"


class DRSAcquisitionThread : public QThread
{
	 Q_OBJECT
public:
	DRSAcquisitionThread(QObject *parent = 0);
	~DRSAcquisitionThread();
	bool initDRSAcquisitionThread(DRS* s_drs, DRSBoard* s_board, DRSAcquisitionConfig* s_config, AnalysisConfig* s_analysisConfig, int updateTime=33);
	void reInit(DRSAcquisitionConfig* s_config, AnalysisConfig* s_analysisConfig);
	void processEvent(EventRawData rawEvent, bool outputSample);
	void lockConfig();
	void unlockConfig();
	void setPaused(bool);
 signals:
	 void newProcessedEvents(QVector<EventStatistics*>*);
	 void newEventSample(EventSampleData*);
	 void eventAcquisitionFinished();

 private:
    void run();
	void GetTimeStamp(EventTimestamp &ts);

public slots:
	void stopAcquisition();
	void onUpdateTimerTimeout();
	void onTemperatureUpdated(float temp);
public:

private:
	EventRawData rawData;
	float* tempValArray;
	float* tempFilteredValArray;
	bool channelEnabled[NUM_DIGITIZER_CHANNELS];
	QVector<EventStatistics*>* processedEvents;
	QTimer* updateTimer = nullptr;
	bool sampleNextEvent = true;
	bool acquiring=false;
	DRS* drs=nullptr;
	DRSBoard* board=nullptr;
	DRSAcquisitionConfig* config=nullptr;
	AnalysisConfig* analysisConfig=nullptr;
	int numEvents=0;
	EventTimestamp firstEventTimestamp;
	//temperature monitoring
	float currentTemp=-1;

	QMutex drsObjectMutex;
	QMutex configMutex;
	QMutex pauseMutex;

	QMutex processedEventsMutex;
	bool requiresPause = false;
};

