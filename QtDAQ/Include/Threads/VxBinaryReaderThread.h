#pragma once

#include <QThread>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QVector>
#include <QMutex>
#include <atomic>
#include "AcquisitionDefinitions.h"
#include "AnalysisConfig.h"
#include <zlib.h>

class VxBinaryReaderThread : public QThread
{    
	 Q_OBJECT
public:
	VxBinaryReaderThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, int s_bufferLength = 1024, QObject *parent = 0);
	 ~VxBinaryReaderThread();
     bool initVxBinaryReaderThread(QString filename, bool isCompressedHeader, int s_runIndex, int updateTime = 250);
	bool isReading();
 signals:
	 void newRawEvents(QVector<EventVx*>*);
	 void filePercentUpdate(float filePercent);
	 void eventReadingFinished();

private:
    void run();
	void swapBuffers();
    qint64 getFileSize(QString filename);
    bool unpackChannel(u_int16_t* chData, u_int16_t channelSize, int8_t* sourceData, u_int16_t firstVal);
#ifdef DEBUG_PACKING
    bool packChannel(u_int16_t* chData, u_int16_t channelSize, int8_t* destData);
    bool WriteDataHeaderPacked();
    bool AppendEventPacked(EventVx* ev);
    gzFile outputFileCompressed = nullptr;
#endif

#ifdef DEBUG_LZO
    bool AppendEventLZO(EventVx* ev);
    bool WriteDataHeaderLZO();

    QFile* lzoFile=nullptr;
#endif

public slots:
	void stopReading(bool forceExit);
	void rewindFile(int s_runIndex);
	void onUpdateTimerTimeout();
	void setPaused(bool paused);
	
private:
	int eventIndex;
    FileFormat fileFormat;
	DataHeader header;    
    gzFile inputFileCompressed = nullptr;
    QFile* inputFileUncompressed = nullptr;
    uchar* uncompressedBuffer=nullptr;
    uchar* lzoBuffer=nullptr;
    uchar* lzoWorkMem=nullptr;
    int lzoBufferSize=-1;
    int uncompressedBufferSize=-1;
	QString filename;
    int8_t* packedData=nullptr;
    int packedDataLength=-1;
	int numEventsInFile;
	int numEventsRead;
	QTimer updateTimer;
    QTime startTime;
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
