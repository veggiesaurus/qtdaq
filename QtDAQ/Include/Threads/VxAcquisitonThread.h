#pragma once

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <atomic>

#include "AcquisitionDefinitions.h"
#include <CAENDigitizer.h>
#include "globals.h"
#include "zlib/zlib.h"

class VxAcquisitionThread : public QThread
{
	Q_OBJECT
public:
	VxAcquisitionThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, QObject *parent = 0);
	~VxAcquisitionThread();
	bool initVxAcquisitionThread(QString config, int s_runIndex, int updateTime = 125);
	bool setFileOutput(QString filename, bool useCompression = true);
signals:
	void newRawEvents(QVector<EventVx*>*);
	void aquisitionError(CAEN_DGTZ_ErrorCode errorCode);

private:
	void run();

	public slots:
	void onUpdateTimerTimeout();
	void setPaused(bool paused);

private:
	int eventIndex;
	DataHeader header;

	QString filename;
	int numEventsAcquired;
	QTimer updateTimer;
	bool isAcquiring;
	bool requiresPause;
	QMutex pauseMutex;


	QMutex* rawMutexes[2];
	EventVx* rawBuffers[2];
	int currentBufferIndex;
	int currentBufferPosition;

	//run tracking
	QMutex runIndexMutex;
	std::atomic<int> runIndex;
};
