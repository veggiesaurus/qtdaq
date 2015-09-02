#pragma once

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <atomic>
#include "zlib/zlib.h"
#include <CAENDigitizer.h>
#include "AcquisitionDefinitions.h"
#include "VxAcquisitionConfig.h"
#include "globals.h"

#define BUFFER_SWAP_TIME 250

class VxAcquisitionThread : public QThread
{
	Q_OBJECT
public:
	VxAcquisitionThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, QObject *parent = 0);
	~VxAcquisitionThread();
	CAENErrorCode initVxAcquisitionThread(VxAcquisitionConfig* s_config, int s_runIndex, int updateTime = 125);
	bool setFileOutput(QString filename, bool useCompression = true);
	bool reInit(VxAcquisitionConfig* s_config);
signals:
	void newRawEvents(QVector<EventVx*>*);
	void aquisitionError(CAEN_DGTZ_ErrorCode errorCode);

private:
	void run();
	void swapBuffers();
	CAENErrorCode ResetDigitizer();
	CAENErrorCode InitDigitizer();
	CAENErrorCode ProgramDigitizer();
	void CloseDigitizer(bool finalClose = false);
	CAENErrorCode AllocateEventStorage();

	CAEN_DGTZ_ErrorCode GetMoreBoardInfo();
	CAEN_DGTZ_ErrorCode WriteRegisterBitmask(uint32_t address, uint32_t data, uint32_t mask);


public slots:
	void setPaused(bool paused);
	void stopAcquisition(bool forceExit);
private:
	VxAcquisitionConfig* config = nullptr;
	int handle = -1; 
	CAEN_DGTZ_BoardInfo_t* boardInfo = nullptr;
	CAEN_DGTZ_EventInfo_t eventInfo;
	uint32_t allocatedSize;
	uint32_t bufferSize;
	uint32_t numEvents;
	char* buffer = nullptr;

	CAEN_DGTZ_UINT16_EVENT_t* event16 = NULL;
	char* eventPtr = nullptr;
	uint32_t previousNumSamples = 0;
	CAENStatus digitizerStatus = STATUS_CLOSED;

	int eventIndex = 0;
	DataHeader header;

	QString filename;
	int numEventsAcquired = 0;
	bool isAcquiring;
	bool requiresPause;
	QMutex pauseMutex;
	QMutex digitizerMutex;
	uint32_t numEventsCaptured =0;

	QMutex* rawMutexes[2];
	EventVx* rawBuffers[2];
	int currentBufferIndex;
	int currentBufferPosition;
	QTime timeSinceLastBufferSwap;
	//run tracking

	QMutex runIndexMutex;
	std::atomic<int> runIndex;
};