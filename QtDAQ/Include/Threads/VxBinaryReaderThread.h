#pragma once

#include <time.h>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QFile>
#include <QMutex>
#include <atomic>
#include "AcquisitionDefinitions.h"
#include "AnalysisConfig.h"
#include "zlib/zlib.h"


class VxBinaryReaderThread : public QThread
{
	 Q_OBJECT
public:
	VxBinaryReaderThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, int s_bufferLength = 1024, QObject *parent = 0);
	 ~VxBinaryReaderThread();
	 bool initVxBinaryReaderThread(QString filename, bool isCompressedInput, int s_runIndex, int updateTime = 250);
	bool isReading();
 signals:
	 void newRawEvents(QVector<EventVx*>*);
	 void filePercentUpdate(float filePercent);
	 void eventReadingFinished();

private:
    void run();
	void swapBuffers();
	qint64 getFileSize(QString filename);
 
public slots:
	void stopReading(bool forceExit);
	void rewindFile(int s_runIndex);
	void onUpdateTimerTimeout();
	void setPaused(bool paused);
	
private:
	int eventIndex;
	DataHeader header;

	gzFile inputFileCompressed;
	QString filename;
	int numEventsInFile;
	int numEventsRead;
	QTimer updateTimer;
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
	int bufferLength;

	//vx1742 switches
	bool vx1742Mode;

	//percentage tracking
	qint64 fileLength;
	float filePercent;

	//run tracking
	QMutex runIndexMutex;
	std::atomic<int> runIndex;
};