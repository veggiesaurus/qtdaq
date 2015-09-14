#include <QTime>
#include <QDebug>
#include <time.h>
#include <lzo/lzo1x.h>
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

bool VxBinaryReaderThread::initVxBinaryReaderThread(QString s_filename, bool isCompressedHeader, int s_runIndex, int updateTime)
{	
    startTime.restart();
	filename=s_filename;
	runIndex = s_runIndex; 

    if (inputFileCompressed)
    {
        gzclose(inputFileCompressed);
        SAFE_DELETE(inputFileCompressed);
    }
    if (inputFileUncompressed)
    {
        inputFileUncompressed->close();
        SAFE_DELETE(inputFileUncompressed);
    }

	if (isReadingFile)	        
        doExitReadLoop=true;

    if (isCompressedHeader)
        fileFormat = GZIP_COMPRESSED;
    else
        fileFormat = LZO_COMPRESSED;

	fileLength = getFileSize(filename);

	filePercent = 0;

    if (fileFormat == GZIP_COMPRESSED || fileFormat == GZIP_COMPRESSED_PACKED)
    {
        inputFileCompressed = gzopen(filename.toLatin1(), "rb");
        if (gzread(inputFileCompressed, &header, sizeof(DataHeader))!=sizeof(DataHeader))
        {
            //problem with the header file!
            gzclose(inputFileCompressed);
            SAFE_DELETE(inputFileUncompressed);
            return false;
        }
    }
    else
    {
        inputFileUncompressed=new QFile(filename);
        inputFileUncompressed->open(QFile::ReadOnly);
        if (inputFileUncompressed->read((char*)&header, (qint64)sizeof(DataHeader))!=sizeof(DataHeader))
        {
            //problem with the header file!
            inputFileUncompressed->close();
            SAFE_DELETE(inputFileUncompressed);
            return false;
        }
    }
#ifdef DEBUG_PACKING
    if (filename.endsWith(".dtz"))
    {
        QString outputFilename = filename.replace(".dtz", ".pcz");
        outputFileCompressed=gzopen(outputFilename.toStdString().c_str(), "wb1f");
        WriteDataHeaderPacked();
    }
#endif
#ifdef DEBUG_LZO
    if (filename.endsWith(".dtz"))
    {
        QString outputFilename = filename.replace(".dtz", ".dlz");
        lzoFile = new QFile(outputFilename);
        lzoFile->open(QFile::WriteOnly);
        WriteDataHeaderLZO();
    }
#endif
    if (inputFileCompressed)
        fileFormat = (strcmp(header.magicNumber, "UCT001P")==0)?GZIP_COMPRESSED_PACKED:GZIP_COMPRESSED;
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

    while (((inputFileCompressed && !gzeof(inputFileCompressed)) || (inputFileUncompressed && !inputFileUncompressed->atEnd())) && !doExitReadLoop)
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
            if (fileFormat == GZIP_COMPRESSED || fileFormat == GZIP_COMPRESSED_PACKED)
            {
                gzrewind(inputFileCompressed);
                if (gzread(inputFileCompressed, &header, sizeof(DataHeader))!=sizeof(DataHeader))
                {
                    //problem with the header file!
                    gzclose(inputFileCompressed);
                    SAFE_DELETE(inputFileUncompressed);
                    break;
                    isReadingFile=true;
                }
            }
            else
            {
                inputFileUncompressed->reset();
                if (inputFileUncompressed->read((char*)&header, (qint64)sizeof(DataHeader))!=sizeof(DataHeader))
                {
                    //problem with the header file!
                    inputFileUncompressed->close();
                    SAFE_DELETE(inputFileUncompressed);
                    break;
                    isReadingFile=true;
                }
            }

			numEventsRead=0;
			doRewindFile=false;
		}
		//read

		//release and swap buffers when position overflows
		if (currentBufferPosition >= bufferLength)
			swapBuffers();

        qint64 byteCount;
        if (inputFileCompressed)
            byteCount = gzoffset64(inputFileCompressed);
        else
            byteCount = inputFileUncompressed->pos();
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

        if (inputFileUncompressed)
        {
            lzo_uint compressedSize;
            uint32_t uncompressedSize;
            inputFileUncompressed->read((char*)&compressedSize, (qint64)(sizeof(lzo_uint)));
            inputFileUncompressed->read((char*)&uncompressedSize, (qint64)(sizeof(uint32_t)));
            if ((int)compressedSize>lzoBufferSize)
            {
                SAFE_DELETE_ARRAY(lzoBuffer);
                lzoBuffer = new uchar[compressedSize];
                lzoBufferSize = compressedSize;
            }
            if ((int)uncompressedSize>uncompressedBufferSize)
            {
                SAFE_DELETE_ARRAY(uncompressedBuffer);
                uncompressedBuffer = new uchar[uncompressedSize];
                uncompressedBufferSize=uncompressedSize;
            }
            if (!lzoWorkMem)
                lzoWorkMem=new uchar[LZO1X_1_MEM_COMPRESS];

            if (inputFileUncompressed->read((char*)lzoBuffer, compressedSize)!=compressedSize)
            {
                freeVxEvent(ev);
                break;
            }
            lzo_uint decompressedSize;
            lzo1x_decompress(lzoBuffer,lzoBufferSize,uncompressedBuffer,&decompressedSize,lzoWorkMem);
            if (decompressedSize!=uncompressedSize)
            {
                freeVxEvent(ev);
                break;
            }
            ev.runIndex = runIndex.load();
            vx1742Mode=false;
            header.MSPS=4000;

            /*uint32_t uPos=0;
            memcpy(&(uncompressedBuffer[uPos]), &ev->info, sizeof(CAEN_DGTZ_EventInfo_t));
            uPos+=sizeof(CAEN_DGTZ_EventInfo_t);
            memcpy(&(uncompressedBuffer[uPos]), ev->data.ChSize, sizeof(uint32_t)*numCh);
            uPos+=(sizeof(uint32_t)*numCh);

            for (int i=0;i<numCh;i++)
            {
                if (ev->data.ChSize[i])
                {
                    memcpy(&(uncompressedBuffer[uPos]), ev->data.DataChannel[i], sizeof(uint16_t) * ev->data.ChSize[i]);
                    uPos+=sizeof(uint16_t) * ev->data.ChSize[i];
                }
            }*/

            uint32_t uPos=0;
            int numCh=2;
            memcpy(&ev.info, &(uncompressedBuffer[uPos]), sizeof(CAEN_DGTZ_EventInfo_t));
            uPos+=sizeof(CAEN_DGTZ_EventInfo_t);
            memcpy(ev.data.ChSize, &(uncompressedBuffer[uPos]), sizeof(uint32_t)*numCh);
            uPos+=(sizeof(uint32_t)*numCh);
            for (int i=0;i<numCh;i++)
            {
                if (ev.data.ChSize[i])
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

                    memcpy(ev.data.DataChannel[i], &(uncompressedBuffer[uPos]), sizeof(uint16_t) * ev.data.ChSize[i]);
                    uPos+=sizeof(uint16_t) * ev.data.ChSize[i];
                }
            }           
        }
        else
        {
            if (gzread(inputFileCompressed, &ev.info, sizeof(CAEN_DGTZ_EventInfo_t))!=sizeof(CAEN_DGTZ_EventInfo_t))
            {
                freeVxEvent(ev);
                break;
            }

            ev.runIndex = runIndex.load();
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
                if (fileFormat == GZIP_COMPRESSED_PACKED)
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
#ifdef DEBUG_LZO
        if (lzoFile)
            AppendEventLZO(&ev);
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
#ifdef DEBUG_LZO
    if (lzoFile)
    {
        lzoFile->close();
        lzoFile=nullptr;
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

#ifdef DEBUG_LZO

bool VxBinaryReaderThread::WriteDataHeaderLZO()
{
    if (lzoFile)
    {
        DataHeader newHeader = header;
        sprintf(newHeader.magicNumber, "UCT001L");
        lzoFile->write((const char*)&newHeader, (qint64)sizeof(DataHeader));
        return true;
    }
    else
        return false;
}

bool VxBinaryReaderThread::AppendEventLZO(EventVx* ev)
{
    //hack for two channels
    int numCh=2;
    int newUncompressedSize=sizeof(CAEN_DGTZ_EventInfo_t)+sizeof(uint32_t)*numCh+numCh*(qMax(ev->data.ChSize[0], ev->data.ChSize[1])*sizeof(uint16_t));
    int newLzoSize=(newUncompressedSize + newUncompressedSize / 16 + 64 + 3);
    if (newUncompressedSize!=uncompressedBufferSize)
    {
        SAFE_DELETE_ARRAY(uncompressedBuffer);
        uncompressedBuffer=new uchar[newUncompressedSize];
        uncompressedBufferSize=newUncompressedSize;
    }
    if (newLzoSize!=lzoBufferSize)
    {
        SAFE_DELETE_ARRAY(lzoBuffer);
        lzoBuffer=new uchar[newLzoSize];
        lzoBufferSize=newLzoSize;
    }
    if (!lzoWorkMem)
        lzoWorkMem=new uchar[LZO1X_1_MEM_COMPRESS];
    uint32_t uPos=0;
    memcpy(&(uncompressedBuffer[uPos]), &ev->info, sizeof(CAEN_DGTZ_EventInfo_t));
    uPos+=sizeof(CAEN_DGTZ_EventInfo_t);
    memcpy(&(uncompressedBuffer[uPos]), ev->data.ChSize, sizeof(uint32_t)*numCh);
    uPos+=(sizeof(uint32_t)*numCh);

    for (int i=0;i<numCh;i++)
    {
        if (ev->data.ChSize[i])
        {
            memcpy(&(uncompressedBuffer[uPos]), ev->data.DataChannel[i], sizeof(uint16_t) * ev->data.ChSize[i]);
            uPos+=sizeof(uint16_t) * ev->data.ChSize[i];
        }
    }
    lzo_uint compressedSize;
    lzo1x_1_compress(uncompressedBuffer, uPos, lzoBuffer, &compressedSize, lzoWorkMem);
    lzoFile->write((const char*)&compressedSize, (qint64)(sizeof(lzo_uint)));
    lzoFile->write((const char*)&uPos, (qint64)(sizeof(uint32_t)));
    lzoFile->write((const char*)lzoBuffer, (qint64)compressedSize);
    return true;
}
#endif
