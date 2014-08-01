#pragma once

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include "DRS4Acquisition.h"
#include "DRS4/DRS.h"
#include "ProcessRoutines.h"
#include "AcquisitionConfig.h"
#include "AnalysisConfig.h"
#include "globals.h"


class DRSAcquisitionThread : public QThread
{
	 Q_OBJECT
public:
	 DRSAcquisitionThread(QObject *parent = 0);	 
	 bool initDRSAcquisitionThread(DRS* s_drs, DRSBoard* s_board, AcquisitionConfig* s_config, AnalysisConfig* s_analysisConfig, int updateTime=100);
	void processEvent(EventRawData rawEvent, bool outputSample);
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
private:
	EventRawData rawData;
	float* tempValArray;
	float* tempFilteredValArray;
	bool channelEnabled[NUM_DIGITIZER_CHANNELS];
    QVector<EventStatistics*>* processedEvents;
	QTimer* updateTimer;
	bool sampleNextEvent;
	bool acquiring;
	DRS* drs;
	DRSBoard* board;
	AcquisitionConfig* config;
	AnalysisConfig* analysisConfig;
	int numEvents;
	EventTimestamp firstEventTimestamp;
	//temperature monitoring
	float currentTemp;
};

