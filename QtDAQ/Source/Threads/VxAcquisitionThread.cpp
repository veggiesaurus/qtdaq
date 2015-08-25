#include "Threads/VxAcquisitonThread.h"

VxAcquisitionThread::VxAcquisitionThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, QObject *parent)
	: QThread(parent)
{

}

VxAcquisitionThread::~VxAcquisitionThread()
{

}

bool VxAcquisitionThread::initVxAcquisitionThread(QString config, int s_runIndex, int updateTime)
{	
	runIndex = s_runIndex;
	return true;
}

void VxAcquisitionThread::run()
{
	
}

void VxAcquisitionThread::onUpdateTimerTimeout()
{

}

void VxAcquisitionThread::setPaused(bool paused)
{
	pauseMutex.lock();
	requiresPause = paused;
	pauseMutex.unlock();
}

bool VxAcquisitionThread::setFileOutput(QString filename, bool useCompression)
{

}