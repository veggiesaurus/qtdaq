#include "ProcessRoutines.h"
#include "vector/vectorclass.h"
#include "globals.h"
#include "Threads/DRSBinaryReaderThread.h"

DRSBinaryReaderThread::DRSBinaryReaderThread(QObject *parent)
	: QThread(parent)
{
	memset(channelEnabled, 0, sizeof(bool)*NUM_DIGITIZER_CHANNELS);
	
	//tempValArray=new float[NUM_DIGITIZER_SAMPLES];
	tempValArray=(float*)qMallocAligned(sizeof(float)*NUM_DIGITIZER_SAMPLES_DRS, 16);
	tempFilteredValArray=(float*)qMallocAligned(sizeof(float)*NUM_DIGITIZER_SAMPLES_DRS, 16);
	//tempFilteredValArray=new float[NUM_DIGITIZER_SAMPLES];
	memset(tempValArray, 0, sizeof(float)*NUM_DIGITIZER_SAMPLES_DRS);
	memset(tempFilteredValArray, 0, sizeof(float)*NUM_DIGITIZER_SAMPLES_DRS);
	processedEventsMutex.lock();
	processedEvents=new QVector<EventStatistics*>();
	processedEventsMutex.unlock();
}

DRSBinaryReaderThread::~DRSBinaryReaderThread()
{		
	/*processedEventsMutex.lock();
	if (processedEvents && ungracefulReadExit)
	{
		for (auto& i : *processedEvents)
		{
			SAFE_DELETE(i);
		}
		processedEvents->clear();
		SAFE_DELETE(processedEvents);
	}
	processedEventsMutex.unlock();*/
	
	SAFE_DELETE_ARRAY(buffer);

	qFreeAligned(tempFilteredValArray);
	qFreeAligned(tempValArray);
}
bool DRSBinaryReaderThread::initDRSBinaryReaderThread(QString filename, bool isCompressedInput, int s_runIndex, AnalysisConfig* s_analysisConfig, int updateTime)
{
	runIndex = s_runIndex;
	eventSize=0;
	analysisConfig=s_analysisConfig;
	memset(channelEnabled, 0, sizeof(bool)*NUM_DIGITIZER_CHANNELS);
	memset(rawData, 0, sizeof(EventRawData)*NUM_BUFFERED_EVENTS);

	compressedInput=isCompressedInput;
	fileMutex.lock();
	if (inputFile)
		fclose(inputFile);
	inputFile=fopen(filename.toStdString().data(), "rb");
	if (!inputFile)
			return false;
	fseek(inputFile, 0, SEEK_END);
	long fileSize=ftell(inputFile);
	fseek(inputFile, 0, SEEK_SET);

	unsigned int magicNumber=0;
	if (fread(&magicNumber, 4, 1, inputFile)!=1)
		return false;
	//skip 20 bytes (serial and timestamp)
	fseek ( inputFile ,20,SEEK_CUR);
	//skip time array
	fseek ( inputFile ,NUM_DIGITIZER_SAMPLES_DRS*4,SEEK_CUR);
	bool foundHeader=false;
	bool validFile=false;
	int numChannels=0;
	while (!foundHeader)
	{
		if (fread(&magicNumber, 4, 1, inputFile)!=1)
			return false;
		//check to see if event header magic number has arrived
		if (magicNumber==MAGIC_NUMBER_EVENT_HEADER)
		{
			foundHeader=true;
			if (numChannels)
				validFile=true;
			break;
		}
		//check to see if channel header magic number has arrived (in the form magicNumCh0+N*magicNumDiff, where N is channel number)
		else if ((magicNumber-MAGIC_NUMBER_CHANNEL0_HEADER)%MAGIC_NUMBER_CHANNEL_DIFF==0)
		{
			int chNumber=(magicNumber-MAGIC_NUMBER_CHANNEL0_HEADER)/MAGIC_NUMBER_CHANNEL_DIFF;
			//check if it's already enabled. If it is, then there's a problem with the file format!
			if (channelEnabled[chNumber])
			{
				validFile=false;
				break;
			}
			else
			{
				//enable channel
				channelEnabled[chNumber]=true;
				numChannels++;
				//skip actual data
				fseek ( inputFile ,NUM_DIGITIZER_SAMPLES_DRS*2,SEEK_CUR);
			}
		}
	}

	if (!validFile)
		return false;
	
	
	//calculate size of each event (in bytes):
	//EHDR(4)+serial(4)+timeStamp(20)+tVals(4*N)+numChannels*(CHDR(4)+fVals(4*N))
	eventSize= 4 + 20 + 4*NUM_DIGITIZER_SAMPLES_DRS + numChannels*(4 + 2*NUM_DIGITIZER_SAMPLES_DRS);
	SAFE_DELETE_ARRAY(buffer);
	buffer=new char[eventSize*NUM_BUFFERED_EVENTS];
	numEventsInFile=fileSize/eventSize;

	// point the event raw data structures to the right place.
	// This way, we can just read from disk straight to the buffer, and the raw data structure will be in place.
	// In addition, build-in caching from the OS will take care of the overhead of reading one event at a time
	
	//skip event header

	char*offset=buffer;

	for (int i=0;i<NUM_BUFFERED_EVENTS;i++)
	{
		offset+=4;
		rawData[i].serial=(int*)(offset);
		//move past serial
		offset+=4;
		rawData[i].timestamp=(EventTimestamp*)(offset);
		//move past timestamp
		offset+=16;
		rawData[i].tValues=(float*)(offset);
		//move past time array
		offset+=NUM_DIGITIZER_SAMPLES_DRS*4;
		//run over each channel
		for (int ch=0;ch<NUM_DIGITIZER_CHANNELS;ch++)
		{
			if (channelEnabled[ch])
			{
				//skip channel header		
				offset+=4;
				rawData[i].fValues[ch]=(unsigned short*)(offset);
				//move past value array
				offset+=NUM_DIGITIZER_SAMPLES_DRS*2;
			}
		}
		rawData[i].additional=(void*)offset;
	}	

	//go back to begining of file, ready for reading
	rewind(inputFile);
	fileMutex.unlock();

	numEventsRead=0;
	updateTimer.stop();
	updateTimer.disconnect();
	updateTimer.setInterval(updateTime);
	connect(&updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerTimeout()));
    updateTimer.start();
	return true;
}

void DRSBinaryReaderThread::rewindFile()
{
	fileMutex.lock();
	rewind(inputFile);
	fileMutex.unlock();
	setPaused(false);
}

void DRSBinaryReaderThread::setPaused(bool paused)
{
	pauseMutex.lock();
	requiresPause = paused;
	pauseMutex.unlock();
}

void DRSBinaryReaderThread::run()
{	
	ungracefulReadExit = false;
	setPaused(false);
	
	while (inputFile && !feof(inputFile))
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

		processedEventsMutex.lock();
		if (!processedEvents)
		{
			processedEvents=new QVector<EventStatistics*>;
			//processedEvents->reserve(1000);
			processedEvents->setSharable(true);

		}
		processedEventsMutex.unlock();

		int nextReadSize=NUM_BUFFERED_EVENTS;
		if ((numEventsInFile-numEventsRead)<NUM_BUFFERED_EVENTS)
		{
			nextReadSize=(numEventsInFile-numEventsRead);
			if (nextReadSize == 0)
				break;
		}
		fileMutex.lock();
		if (!inputFile  || fread(buffer, sizeof(char), eventSize*nextReadSize, inputFile) != eventSize*nextReadSize)
		{
			fileMutex.unlock();
			ungracefulReadExit = true;
			break;
		}
		//process event, output each event
		else
		{
			fileMutex.unlock();
			for (int i = 0; i<nextReadSize; i++)
			{
				//sanity checks:
				if (rawData[i].timestamp->year>2010 && rawData[i].timestamp->year<2020)
				{
					//reset first event timestamp
					if (!numEventsRead)
						firstEventTimestamp=*rawData[i].timestamp;

					processEvent(rawData[i], sampleNextEvent);
					numEventsRead++;
				}
			}
		}			
	}
	
	processedEventsMutex.lock();
	if (!ungracefulReadExit)
	{
		if (processedEvents && processedEvents->size() > 0)
			emit newProcessedEvents(processedEvents);
	}
	processedEventsMutex.unlock();
	stopReading(ungracefulReadExit);	
}


void DRSBinaryReaderThread::onUpdateTimerTimeout()
{
	sampleNextEvent=true;
	processedEventsMutex.lock();
	if (processedEvents && processedEvents->size()>0)
	{        
		emit newProcessedEvents(processedEvents);
		processedEvents=new QVector<EventStatistics*>();
		//processedEvents->reserve(1000);
	}
	processedEventsMutex.unlock();
}

void DRSBinaryReaderThread::onTemperatureUpdated(float temp)
{
	//this is just for testing purposes...only acquisition thread actually needs temp
	currentTemp=temp;
}

void DRSBinaryReaderThread::stopReading(bool forceExit)
{	
	updateTimer.stop();
	ungracefulReadExit = forceExit;
	fileMutex.lock();
	if (inputFile)
	{
		fclose(inputFile);
		inputFile = NULL;
	}
	fileMutex.unlock();
	//only emit if we're finished reading without manual exit
	if (!forceExit)
		emit eventReadingFinished();
}

void DRSBinaryReaderThread::processEvent(EventRawData rawEvent, bool outputSample)
{	
	EventStatistics* stats=new EventStatistics();
	stats->timestamp=*rawEvent.timestamp;
	stats->serial=*rawEvent.serial;
	stats->runIndex = runIndex;
	bool processSuccess = true;
	
	int sampleRateMSPS = (int)round((NUM_DIGITIZER_SAMPLES_DRS - 1)*1000.0f / (rawEvent.tValues[NUM_DIGITIZER_SAMPLES_DRS - 1] - rawEvent.tValues[0]));

	EventSampleData* sample;
	if (outputSample)
	{
		sample=new EventSampleData();
		//set null pointers for channel initially
		memset(sample, 0, sizeof(EventSampleData));
		memset(sample->fValues, 0, NUM_DIGITIZER_CHANNELS*sizeof(float*));		
	}


	//analysisConfig->displayCFDSignal = true;

	for (int ch=0;ch<NUM_DIGITIZER_CHANNELS;ch++)
	{		
		if (rawEvent.fValues[ch])
		{
			stats->channelStatistics[ch].channelNumber=ch;
//SSE2 optimisation
#if NUM_DIGITIZER_SAMPLES%4==0
			float* stPtr=tempValArray;
			unsigned short* ldPtr=rawEvent.fValues[ch];
			Vec8us vFVal;
			Vec4f vValArray;
			Vec4f constRescale=1.0/64.0;
			for (int i=0;i<NUM_DIGITIZER_SAMPLES_DRS/4;i++,ldPtr+=4,stPtr+=4)
			{				
				vFVal.load(ldPtr);
				(to_float(extend_low(vFVal))*constRescale).store(stPtr);				
			}
#else
			for (int i=0;i<NUM_DIGITIZER_SAMPLES_DRS;i++)
				tempValArray[i]=((float)rawEvent.fValues[ch][i])/64.0f;	
#endif
			
			processSuccess&=clone(tempValArray, NUM_DIGITIZER_SAMPLES_DRS, tempFilteredValArray);
			if (analysisConfig->preCFDFilter)
				processSuccess&=lowPassFilter(tempValArray, NUM_DIGITIZER_SAMPLES_DRS,analysisConfig->preCFDFactor);
			float CFDThreshold=512.0f;
			processSuccess&=findBaseline(tempValArray, 0, analysisConfig->baselineSampleRange, analysisConfig->baselineSampleSize, stats->channelStatistics[ch].baseline);
			//if the digital gain is not unity
			if (abs(analysisConfig->digitalGain-1.00f)>0.01f)
			{				
//SSE optimisation
#if NUM_DIGITIZER_SAMPLES%4==0
				float* stPtr=tempValArray;
				Vec4f vValArray;
				Vec4f vBaseline=stats->channelStatistics[ch].baseline;
				Vec4f vDigitalGain=analysisConfig->digitalGain;
				for (int i=0;i<NUM_DIGITIZER_SAMPLES_DRS/4;i++,stPtr+=4)
				{
					vValArray.load(stPtr);
					vValArray=vBaseline+vDigitalGain*(vValArray-vBaseline);
					vValArray.store(stPtr);
				}
#else
				for (int i=0;i<NUM_DIGITIZER_SAMPLES_DRS;i++)
				{
					tempValArray[i]=stats->channelStatistics[ch].baseline+analysisConfig->digitalGain*(tempValArray[i]-stats->channelStatistics[ch].baseline);
				}	
#endif
			}
			processSuccess&=cfdSampleOptimized(tempValArray, stats->channelStatistics[ch].baseline, NUM_DIGITIZER_SAMPLES_DRS, analysisConfig->CFDFraction, analysisConfig->CFDLength, analysisConfig->CFDOffset, CFDThreshold, 1.f, tempFilteredValArray);
			if (analysisConfig->postCFDFilter)
				processSuccess&=lowPassFilter(tempFilteredValArray, NUM_DIGITIZER_SAMPLES_DRS,analysisConfig->postCFDFactor);

				
			
            int positionOfThresholdCrossing;
			int positionOfCFDMin, positionOfCFDMax;
            float minCFDValue, maxCFDValue;

			processSuccess&=findMinMaxValue(tempFilteredValArray, NUM_DIGITIZER_SAMPLES_DRS, 0, NUM_DIGITIZER_SAMPLES_DRS, minCFDValue, positionOfCFDMin, maxCFDValue, positionOfCFDMax);
                
            processSuccess&=findIntersection(tempFilteredValArray, NUM_DIGITIZER_SAMPLES_DRS, positionOfCFDMin, positionOfCFDMax, 2, CFDThreshold, false, positionOfThresholdCrossing);
            float crossingPositionInterpolated;
			float cfdIndex=0;
            if (positionOfThresholdCrossing>=positionOfCFDMin)
            {
                float slope, offset;
                //linear fit over 4 points of slope
                processSuccess&=linearFit(tempFilteredValArray, NUM_DIGITIZER_SAMPLES_DRS, positionOfThresholdCrossing-2, positionOfThresholdCrossing+1, slope, offset);
				cfdIndex=(CFDThreshold-offset)/slope;
				int indexLow=(int)(cfdIndex);
				float d=cfdIndex-indexLow;
				float time=rawEvent.tValues[indexLow]*(1-d)+(d)*rawEvent.tValues[indexLow+1];
				if (analysisConfig->useTimeCFD)
					stats->channelStatistics[ch].timeOfCFDCrossing=time;
				else
					stats->channelStatistics[ch].timeOfCFDCrossing=cfdIndex/(sampleRateMSPS/1000.0);

				//if there is a previous channel
				if (ch>0 && stats->channelStatistics[ch-1].channelNumber!=-1 && stats->channelStatistics[ch-1].timeOfCFDCrossing>-1000)
				{
					stats->channelStatistics[ch].deltaTprevChannelCFD=stats->channelStatistics[ch].timeOfCFDCrossing-stats->channelStatistics[ch-1].timeOfCFDCrossing;
				}
				else
					//dummy deltaT
					stats->channelStatistics[ch].deltaTprevChannelCFD=-1000;
            }
			else
				stats->channelStatistics[ch].timeOfCFDCrossing=-1000;

			processSuccess&=findMinMaxValue(tempValArray, NUM_DIGITIZER_SAMPLES_DRS, 0, NUM_DIGITIZER_SAMPLES_DRS, stats->channelStatistics[ch].minValue, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].maxValue, stats->channelStatistics[ch].indexOfMax);
			
			if (stats->channelStatistics[ch].indexOfMin>0 && stats->channelStatistics[ch].indexOfMin<NUM_DIGITIZER_SAMPLES_DRS)
				stats->channelStatistics[ch].timeOfMin=rawEvent.tValues[stats->channelStatistics[ch].indexOfMin];

			if (stats->channelStatistics[ch].indexOfMax>0 && stats->channelStatistics[ch].indexOfMax<NUM_DIGITIZER_SAMPLES_DRS)
				stats->channelStatistics[ch].timeOfMax=rawEvent.tValues[stats->channelStatistics[ch].indexOfMax];

			processSuccess&=findHalfRise(tempValArray, NUM_DIGITIZER_SAMPLES_DRS, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].baseline, stats->channelStatistics[ch].indexOfHalfRise);
			if (stats->channelStatistics[ch].indexOfHalfRise>0 && stats->channelStatistics[ch].indexOfHalfRise<NUM_DIGITIZER_SAMPLES_DRS)
				stats->channelStatistics[ch].timeOfHalfRise=rawEvent.tValues[stats->channelStatistics[ch].indexOfHalfRise];

			
			int startOffset = (analysisConfig->startGate*sampleRateMSPS) / 1000;
			int shortGateOffset = (analysisConfig->shortGate*sampleRateMSPS) / 1000;
			int longGateOffset = (analysisConfig->longGate*sampleRateMSPS) / 1000;
			int timeOffset;
			switch (analysisConfig->timeOffset)
			{
			case CFD_TIME:
				timeOffset = (int)cfdIndex;
				break;
			case TIME_HALF_RISE:
				timeOffset = stats->channelStatistics[ch].indexOfHalfRise;
				break;
			default:
				timeOffset = stats->channelStatistics[ch].indexOfMin;
				break;
			}
			
			//processSuccess&=calculateIntegrals(tempValArray, NUM_DIGITIZER_SAMPLES, stats->channelStatistics[ch].baseline, timeOffset+startOffset, timeOffset+shortGateOffset, timeOffset+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
			processSuccess &= calculateIntegralsCorrected(tempValArray, NUM_DIGITIZER_SAMPLES_DRS, stats->channelStatistics[ch].baseline, cfdIndex + startOffset, cfdIndex + shortGateOffset, cfdIndex + longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
			processSuccess &= calculatePSD(stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral, stats->channelStatistics[ch].PSD);

			stats->channelStatistics[ch].secondsFromFirstEvent=getSecondsFromFirstEvent(firstEventTimestamp, *rawEvent.timestamp);

			if (outputSample)
			{
				sample->numSamples = NUM_DIGITIZER_SAMPLES_DRS;
				sample->MSPS = sampleRateMSPS;
				if (!sample->tValues)
				{
					sample->tValues = new float[NUM_DIGITIZER_SAMPLES_DRS];
					for (size_t i = 0; i < NUM_DIGITIZER_SAMPLES_DRS; i++)
						sample->tValues[i] = rawEvent.tValues[i] - rawEvent.tValues[0];
					//processSuccess &= clone(rawEvent.tValues, NUM_DIGITIZER_SAMPLES, sample->tValues);
				}
				sample->fValues[ch] = new float[NUM_DIGITIZER_SAMPLES_DRS];
				//processSuccess&=clone(tempFilteredValArray, numSamples, sample->fValues[ch]);
				if (analysisConfig->displayCFDSignal)
					processSuccess &= clone(tempFilteredValArray, NUM_DIGITIZER_SAMPLES_DRS, sample->fValues[ch]);
				else
					processSuccess &= clone(tempValArray, NUM_DIGITIZER_SAMPLES_DRS, sample->fValues[ch]);

				sample->baseline = stats->channelStatistics[ch].baseline;
				sample->indexStart = timeOffset + startOffset;
				sample->indexShortEnd = timeOffset + shortGateOffset;
				sample->indexLongEnd = timeOffset + longGateOffset;
				sample->cfdTime = timeOffset / (sampleRateMSPS/1000.0f);
			}

		}
		//write -1 to channel number => no data
		else
			stats->channelStatistics[ch].channelNumber=-1;
	}
	if (outputSample)
	{
		sample->stats=*stats;
		emit newEventSample(sample);
		sampleNextEvent = false;
	}
	processedEventsMutex.lock();
	if (processedEvents)
		processedEvents->push_back(stats);
	processedEventsMutex.unlock();
}