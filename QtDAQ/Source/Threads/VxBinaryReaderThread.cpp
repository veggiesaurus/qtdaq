#include <QFile>
#include <QTime>
#include <QDebug>
#include <time.h>
#include "Threads/VxBinaryReaderThread.h"

VxBinaryReaderThread::VxBinaryReaderThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, int s_bufferLength, QObject *parent)
	: QThread(parent)
{	
	doRewindFile=false;
	doExitReadLoop=false;
	isReadingFile=false;
	requiresPause=false;

	rawBuffers[0]=s_rawBuffer1;
	rawBuffers[1]=s_rawBuffer2;
	rawMutexes[0]=s_rawBuffer1Mutex;
	rawMutexes[1]=s_rawBuffer2Mutex;
	bufferLength = s_bufferLength;
}

VxBinaryReaderThread::~VxBinaryReaderThread()
{

}

bool VxBinaryReaderThread::initVxBinaryReaderThread(QString s_filename, bool isCompressedInput, int s_runIndex, int updateTime)
{	
    startTime.restart();
	filename=s_filename;
	runIndex = s_runIndex;

	if (isReadingFile)
	{
		gzclose(inputFileCompressed);
		inputFileCompressed=NULL;

		doExitReadLoop=true;
	}

	if (inputFileCompressed)
		gzclose(inputFileCompressed);
	fileLength = getFileSize(filename);
	inputFileCompressed = gzopen(filename.toLatin1(), "rb");
	filePercent = 0;

	if (!inputFileCompressed)
	{
		inputFileCompressed=NULL;
		return false;
	}
	if (gzread(inputFileCompressed, &header, sizeof(DataHeader))!=sizeof(DataHeader))
	{
		//problem with the header file!
		gzclose(inputFileCompressed);
		inputFileCompressed=NULL;
		return false;
	}
#ifdef DEBUG_PACKING
    if (filename.endsWith(".dtz"))
    {
        QString outputFilename = filename.replace(".dtz", ".pcz");
        outputFileCompressed=gzopen(outputFilename.toStdString().c_str(), "wb1f");
        WriteDataHeaderPacked();
    }
#endif
    inputFilePacked = (strcmp(header.magicNumber, "UCT001P")==0);
    qDebug()<<header.magicNumber;
	numEventsRead=0;
	updateTimer.stop();
	updateTimer.disconnect();

	//start reading into buffer0, but need to lock and unlock buffer1 as well to make sure it's free
	rawMutexes[0]->lock();
	rawMutexes[1]->lock();
	for (int i = 0; i<bufferLength; i++)
	{
		rawBuffers[0][i].processed=false;
		rawBuffers[1][i].processed=false;
	}
	rawMutexes[1]->unlock();
	currentBufferIndex=0;
	currentBufferPosition=0;

	updateTimer.setInterval(updateTime);
	connect(&updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerTimeout()));
    updateTimer.start();
	doExitReadLoop=false;
	return true;
}

void VxBinaryReaderThread::rewindFile(int s_runIndex)
{
	//if eof has been reached
	if (gzeof(inputFileCompressed) || !inputFileCompressed)
	{
		if (inputFileCompressed)
			gzclose(inputFileCompressed);
		numEventsRead=0;
		initVxBinaryReaderThread(filename, true, s_runIndex, updateTimer.interval());
		start();
	}
	else
	{
		runIndex = s_runIndex;
		doRewindFile = true;

	}
}

void VxBinaryReaderThread::run()
{	
	isReadingFile=true;
	currentBufferIndex=0;
	currentBufferPosition=0;
	int eventCounter=0;

	while (inputFileCompressed && !gzeof(inputFileCompressed) && !doExitReadLoop)
	{
		pauseMutex.lock();
		if (requiresPause)
		{
			pauseMutex.unlock();
			msleep(100);
			continue;
		}
		else
			pauseMutex.unlock();		

		
		if (doRewindFile)
		{
			gzrewind(inputFileCompressed);	
			if (gzread(inputFileCompressed, &header, sizeof(DataHeader))!=sizeof(DataHeader))
			{
				//problem with the header file!
				gzclose(inputFileCompressed);
				inputFileCompressed=NULL;
				break;
				isReadingFile=true;
            }
			numEventsRead=0;
			doRewindFile=false;
		}
		//read

		//release and swap buffers when position overflows
		if (currentBufferPosition >= bufferLength)
			swapBuffers();

		qint64 byteCount = gzoffset64(inputFileCompressed);
		//overflow!
		if (byteCount < 0)
			byteCount += 2147483648;
		filePercent = (float)(byteCount) / fileLength;
		EventVx& ev=rawBuffers[currentBufferIndex][currentBufferPosition];
		//retain previous event sizes
		uint32_t prevEventChSize[MAX_UINT16_CHANNEL_SIZE];
		uint32_t prevEventChSizeFloat[MAX_UINT16_CHANNEL_SIZE];
		memcpy(prevEventChSize, ev.data.ChSize, sizeof(uint32_t)*MAX_UINT16_CHANNEL_SIZE);		
		memcpy(prevEventChSizeFloat, ev.fData.ChSize, sizeof(uint32_t)*MAX_UINT16_CHANNEL_SIZE);		
		//ev.processed=false;		
		if (gzread(inputFileCompressed, &ev.info, sizeof(CAEN_DGTZ_EventInfo_t))!=sizeof(CAEN_DGTZ_EventInfo_t))
		{
			freeVxEvent(ev);
			break;
		}
		
		ev.runIndex = runIndex.load();
		//vx1742: BoardId of 2
		if (ev.info.BoardId==2)
		{
			vx1742Mode=true;
			//vx1742 in 1 GSPS mode by default
			header.MSPS=1000;

			uint8_t GrPresent[MAX_X742_GROUP_SIZE];
			uint32_t ChSize[MAX_X742_CHANNEL_SIZE];
			int adjustedChNum=0;
			//read which groups are present
			if (gzread(inputFileCompressed, GrPresent, sizeof(uint8_t) * MAX_X742_GROUP_SIZE)!= sizeof(uint8_t) * MAX_X742_GROUP_SIZE)
			{
				freeVxEvent(ev);
				break;
			}
			for (int gr=0;gr<MAX_X742_GROUP_SIZE;gr++)
			{
				if (GrPresent[gr])
				{
					//if there is a group present, read the channel sizes out
					if (gzread(inputFileCompressed, ChSize, sizeof(uint32_t) * MAX_X742_CHANNEL_SIZE)!= sizeof(uint32_t) * MAX_X742_CHANNEL_SIZE)
					{
						freeVxEvent(ev);
						break;
					}

					//run through each channel, if channel is present, read data
					for (int ch=0;ch<MAX_X742_CHANNEL_SIZE;ch++)
					{
						if (ChSize[ch])
						{							
							int i=gr*MAX_X742_CHANNEL_SIZE+ch;
							ev.fData.ChSize[i]=ChSize[ch];
							//clear and re-init array if size change is required
							if (ev.fData.ChSize[i]!=prevEventChSizeFloat[i])
							{
								SAFE_DELETE_ARRAY(ev.fData.DataChannel[i]);
								ev.fData.DataChannel[i]=new float[ev.fData.ChSize[i]];
							}
							int s=gzread(inputFileCompressed, ev.fData.DataChannel[i], sizeof(float) * ev.fData.ChSize[i]);
							if (s!=sizeof(float)*ev.fData.ChSize[i])
							{
								freeVxEvent(ev);
								break;
							}
						}
					}

				}
			}
		}
		else
		{
			vx1742Mode=false;
			header.MSPS=4000;

			if (gzread(inputFileCompressed, &ev.data.ChSize, sizeof(uint32_t) * MAX_UINT16_CHANNEL_SIZE)!= sizeof(uint32_t) * MAX_UINT16_CHANNEL_SIZE)
			{
				freeVxEvent(ev);
				break;
			}
			//error handling: bad event
			if (ev.info.BoardId>100 || ev.info.EventSize>200000)
			{
				freeVxEvent(ev);
				break;
			}
			for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
			{
				if (!ev.data.ChSize[i])
					continue;
				//error handling: bad event
				if (ev.data.ChSize[0]>75000)
				{
					freeVxEvent(ev);
					break;
				}

				//clear and re-init array if size change is required
				if (ev.data.ChSize[i]!=prevEventChSize[i])
				{
					SAFE_DELETE_ARRAY(ev.data.DataChannel[i]);
					ev.data.DataChannel[i]=new uint16_t[ev.data.ChSize[i]];
				}

                bool readSuccess;
                if (inputFilePacked)
                {
                    if (ev.data.ChSize[i]-1 != packedDataLength)
                    {
                        SAFE_DELETE_ARRAY(packedData);
                        packedDataLength = ev.data.ChSize[i]-1;
                        packedData = new int8_t[packedDataLength];
                    }
                    u_int16_t firstVal=0;
                    readSuccess = gzread(inputFileCompressed, &firstVal, sizeof(u_int16_t))==sizeof(u_int16_t);
                    readSuccess &= gzread(inputFileCompressed, packedData, sizeof(u_int8_t) * packedDataLength)==sizeof(u_int8_t) * packedDataLength;
                    if (readSuccess)
                        readSuccess&= unpackChannel(ev.data.DataChannel[i], ev.data.ChSize[i], packedData, firstVal);
                }
                else
                    readSuccess = gzread(inputFileCompressed, ev.data.DataChannel[i], sizeof(uint16_t) * ev.data.ChSize[i])==sizeof(uint16_t)*ev.data.ChSize[i];

                if (!readSuccess)
				{
					freeVxEvent(ev);
					break;
				}
			}
        }
#ifdef DEBUG_PACKING
        if (outputFileCompressed)
            AppendEventPacked(&ev);
#endif
		ev.processed=false;
		currentBufferPosition++;
		numEventsRead++;		
    }
#ifdef DEBUG_PACKING
    if (outputFileCompressed)
    {
        gzflush(outputFileCompressed, Z_FINISH);
        gzclose(outputFileCompressed);
        outputFileCompressed=nullptr;
    }
#endif
	rawMutexes[currentBufferIndex]->unlock();
	//swap and lock other buffer to prevent re-processing of data
	currentBufferIndex=1-currentBufferIndex;
	//rawMutexes[currentBufferIndex]->lock();
	stopReading(false);	
}

void VxBinaryReaderThread::onUpdateTimerTimeout()
{
	emit filePercentUpdate(filePercent);
}

void VxBinaryReaderThread::setPaused(bool paused)
{
	pauseMutex.lock();
	requiresPause=paused;
	pauseMutex.unlock();
}

void VxBinaryReaderThread::stopReading(bool forceExit)
{
    qDebug()<<numEventsRead<<" events read in "<<startTime.elapsed()<<" ms ("<<(1000.0*numEventsRead/qMax(startTime.elapsed(), 1))<<" events/s";
	if (inputFileCompressed)
	{
		gzclose(inputFileCompressed);
		inputFileCompressed=NULL;
	}
	isReadingFile=false;
	//only emit if we're finished reading without manual exit
	if (!forceExit)
		emit eventReadingFinished();
}

bool VxBinaryReaderThread::isReading()
{
	return isReadingFile;
}

qint64 VxBinaryReaderThread::getFileSize(QString filename)
{
	QFile f(filename);
	qint64 fileSize = f.size();
	f.close();
	return fileSize;
}

bool VxBinaryReaderThread::unpackChannel(u_int16_t* chData, u_int16_t channelSize, int8_t* sourceData, u_int16_t firstVal)
{
    chData[0]=firstVal;
    for (auto i = 0;i<channelSize-1;i++)
        chData[i+1] = chData[i]+sourceData[i];
    return true;
}

#ifdef DEBUG_PACKING

bool VxBinaryReaderThread::WriteDataHeaderPacked()
{
    DataHeader newHeader = header;
    sprintf(newHeader.magicNumber, "UCT001P");
    if (!gzwrite(outputFileCompressed, &newHeader, sizeof(DataHeader)))
        return false;
    return true;
}

bool VxBinaryReaderThread::AppendEventPacked(EventVx* ev)
{
    if (!gzwrite(outputFileCompressed, &ev->info, sizeof(CAEN_DGTZ_EventInfo_t)))
        return false;
    if (!gzwrite(outputFileCompressed, ev->data.ChSize, sizeof(uint32_t)*MAX_UINT16_CHANNEL_SIZE))
        return false;
    for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
    {
        if (!ev->data.ChSize[i])
            continue;
        if (ev->data.ChSize[i]-1 != packedDataLength)
        {
            SAFE_DELETE_ARRAY(packedData);
            packedDataLength = ev->data.ChSize[i]-1;
            packedData = new int8_t[packedDataLength];
        }
        packChannel(ev->data.DataChannel[i], ev->data.ChSize[i], packedData);
        if (gzwrite(outputFileCompressed, &(ev->data.DataChannel[i][0]), sizeof(uint16_t)) && !gzwrite(outputFileCompressed, packedData, sizeof(u_int8_t) * packedDataLength))
            return false;
    }
    return true;
}

bool VxBinaryReaderThread::packChannel(u_int16_t* chData, u_int16_t channelSize, int8_t* destData)
{
    for (auto i = 1; i<channelSize; i++)
    {
        if (chData[i]-chData[i-1]>MAXINT8)
        {
            qDebug()<<"Error packing channel data";
            return false;
        }
        destData[i-1] = (int8_t)(chData[i]-chData[i-1]);
    }
    return true;
}

#endif


void VxBinaryReaderThread::swapBuffers()
{
#ifdef DEBUG_LOCKS
    qDebug()<<"Acq: Releasing buffer "<<currentBufferIndex;
#endif
    rawMutexes[currentBufferIndex]->unlock();
#ifdef DEBUG_LOCKS
    qDebug()<<"Acq: Released buffer "<<currentBufferIndex;
#endif
    //swap
    currentBufferIndex = 1 - currentBufferIndex;
#ifdef DEBUG_LOCKS
    qDebug()<<"Acq: Locking buffer "<<currentBufferIndex;
#endif
    rawMutexes[currentBufferIndex]->lock();
#ifdef DEBUG_LOCKS
    qDebug()<<"Acq: Locked buffer "<<currentBufferIndex;
#endif
    currentBufferPosition = 0;
}
