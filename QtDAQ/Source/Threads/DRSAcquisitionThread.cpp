#include "Threads/DRSAcquisitionThread.h"

DRSAcquisitionThread::DRSAcquisitionThread(QObject *parent)
	: QThread(parent)
{
	memset(channelEnabled, 0, sizeof(bool)*NUM_DIGITIZER_CHANNELS);
	tempValArray=new float[NUM_DIGITIZER_SAMPLES];
	tempFilteredValArray=new float[NUM_DIGITIZER_SAMPLES];
	memset(tempValArray, 0, sizeof(float)*NUM_DIGITIZER_SAMPLES);
	memset(tempFilteredValArray, 0, sizeof(float)*NUM_DIGITIZER_SAMPLES);
	processedEvents=new QVector<EventStatistics*>;
	processedEvents->setSharable(true);

}

bool DRSAcquisitionThread::initDRSAcquisitionThread(DRS* s_drs, DRSBoard* s_board, AcquisitionConfig* s_config, AnalysisConfig* s_analysisConfig, int updateTime)
{
	memset(channelEnabled, 0, sizeof(bool)*NUM_DIGITIZER_CHANNELS);
	memset(&rawData, 0, sizeof(EventRawData));
	config=s_config;
	analysisConfig=s_analysisConfig;
	drs=s_drs;
	board=s_board;
	board->Init();
	config->SetDRS4Config(board);
	numEvents=0;
	

	if (updateTimer)
	{
		updateTimer->stop();
		updateTimer->disconnect();
		SAFE_DELETE(updateTimer);
	}
	updateTimer=new QTimer();
	updateTimer->setInterval(updateTime);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerTimeout()));
	updateTimer->start();
    
	return true;
}

void DRSAcquisitionThread::run()
{	
	numEvents=0;
	EventRawData rawData;
	rawData.tValues=new float[NUM_DIGITIZER_SAMPLES];
	int firstChannel=8, lastChannel=0;
	//find which is the first channel to read out, and which is the last
	//(only read out channels that are required, to improve throughput)
	for (int i=0;i<NUM_DIGITIZER_CHANNELS;i++)
	{
		if (config->channelEnabled[i])
		{
			firstChannel=std::min(firstChannel, i);
			lastChannel=std::max(lastChannel, i);
			rawData.fValues[i]=new unsigned short[NUM_DIGITIZER_SAMPLES];
		}
		else
			rawData.fValues[i]=NULL;
	}


	float* tmpSamples=new float[NUM_DIGITIZER_SAMPLES];
	//todo: error handling if no channels selected

	if (firstChannel>lastChannel)
		return;

	acquiring=true;
	int serial=board->GetBoardSerialNumber();
	EventTimestamp timeStamp;
	while(acquiring)
	{
		if (!processedEvents)
		{
			processedEvents=new QVector<EventStatistics*>;
			processedEvents->setSharable(true);

		}

		/* start board (activate domino wave) */
		board->StartDomino();

		/* wait for trigger */
		while (board->IsBusy());

		/* read all waveforms */
		board->TransferWaves(0, 8);

		/* read time (X) array in ns */
		board->GetTime(0, board->GetTriggerCell(0), rawData.tValues);
		rawData.serial=&serial;
		GetTimeStamp(timeStamp);
		//reset first event timestamp
		if (!numEvents)
			firstEventTimestamp=timeStamp;
		rawData.timestamp=&timeStamp;
		/* decode waveform (Y) array first channel in mV */
		for (int i=0;i<NUM_DIGITIZER_CHANNELS;i++)
		{
			if (config->channelEnabled[i])
			{
				//DRS4 offset (ch1 is in index 0, ch2 is in index 2, etc)
				//board->GetRawWave(0, i*2, rawData.fValues[i]);
				board->GetWave(0, i*2,tmpSamples);
				for (int j=0;j<NUM_DIGITIZER_SAMPLES;j++)
					rawData.fValues[i][j]=(unsigned short)((tmpSamples[j]+512)*65536/1024.0);
			}
		}
		numEvents++;
		processEvent(rawData, sampleNextEvent);
		sampleNextEvent=false;
	}

}

void DRSAcquisitionThread::GetTimeStamp(EventTimestamp &ts)
{
#ifdef _MSC_VER
	SYSTEMTIME t;
	static unsigned int ofs = 0;

	GetLocalTime(&t);
	if (ofs == 0)
		ofs = timeGetTime() - t.wMilliseconds;
	ts.year         = t.wYear;
	ts.month        = t.wMonth;
	ts.day          = t.wDay;
	ts.hour         = t.wHour;
	ts.minute       = t.wMinute;
	ts.second       = t.wSecond;
	ts.millisecond = (timeGetTime() - ofs) % 1000;
#else
	struct timeval t;
	struct tm *lt;
	time_t now;

	gettimeofday(&t, NULL);
	time(&now);
	lt = localtime(&now);

	ts.year         = lt->tm_year+1900;
	ts.month        = lt->tm_mon+1;
	ts.day          = lt->tm_mday;
	ts.hour         = lt->tm_hour;
	ts.minute       = lt->tm_min;
	ts.second       = lt->tm_sec;
	ts.millisecond = t.tv_usec/1000;
#endif /* OS_UNIX */
}


void DRSAcquisitionThread::onUpdateTimerTimeout()
{
	sampleNextEvent=true;
	if (processedEvents && processedEvents->size()>0)
	{        
		emit newProcessedEvents(processedEvents);
		processedEvents=new QVector<EventStatistics*>();
		processedEvents->setSharable(true);
	}
}

void DRSAcquisitionThread::onTemperatureUpdated(float temp)
{
	currentTemp=temp;
}

void DRSAcquisitionThread::processEvent(EventRawData rawEvent, bool outputSample)
{
	EventStatistics* stats=new EventStatistics();
	stats->timestamp=*rawEvent.timestamp;
	stats->serial=*rawEvent.serial;	
	bool processSuccess=true;

	EventSampleData* sample;
	if (outputSample)
	{
		sample=new EventSampleData();
		//set null pointers for channel initially
		memset(sample, 0, sizeof(EventSampleData));
		memset(sample->fValues, 0, NUM_DIGITIZER_CHANNELS*sizeof(float*));
		sample->tValues=new float[NUM_DIGITIZER_SAMPLES];
		processSuccess&=clone(rawEvent.tValues, NUM_DIGITIZER_SAMPLES, sample->tValues);
	}
	
	for (int ch=0;ch<NUM_DIGITIZER_CHANNELS;ch++)
	{		
		if (rawEvent.fValues[ch])
		{
			stats->channelStatistics[ch].channelNumber=ch;
			stats->channelStatistics[ch].temperature=currentTemp;
			//SSE2 optimisation
#if NUM_DIGITIZER_SAMPLES%4==0
			float* stPtr=tempValArray;
			unsigned short* ldPtr=rawEvent.fValues[ch];
			Vec8us vFVal;
			Vec4f vValArray;
			Vec4f constRescale=1.0/64.0;
			for (int i=0;i<NUM_DIGITIZER_SAMPLES/4;i++,ldPtr+=4,stPtr+=4)
			{				
				vFVal.load(ldPtr);
				(to_float(extend_low(vFVal))*constRescale).store(stPtr);				
			}
#else
			for (int i=0;i<NUM_DIGITIZER_SAMPLES;i++)
				tempValArray[i]=((float)rawEvent.fValues[ch][i])/64.0f;	
#endif
			
			processSuccess&=clone(tempValArray, NUM_DIGITIZER_SAMPLES, tempFilteredValArray);
			if (analysisConfig->preCFDFilter)
				processSuccess&=lowPassFilter(tempValArray, NUM_DIGITIZER_SAMPLES,analysisConfig->preCFDFactor);
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
				for (int i=0;i<NUM_DIGITIZER_SAMPLES/4;i++,stPtr+=4)
				{
					vValArray.load(stPtr);
					vValArray=vBaseline+vDigitalGain*(vValArray-vBaseline);
					vValArray.store(stPtr);
				}
#else
				for (int i=0;i<NUM_DIGITIZER_SAMPLES;i++)
				{
					tempValArray[i]=stats->channelStatistics[ch].baseline+analysisConfig->digitalGain*(tempValArray[i]-stats->channelStatistics[ch].baseline);
				}	
#endif
			}		
			processSuccess&=cfdSampleOptimized(tempValArray, stats->channelStatistics[ch].baseline, NUM_DIGITIZER_SAMPLES, analysisConfig->CFDFraction, analysisConfig->CFDLength, analysisConfig->CFDOffset, CFDThreshold, 1.f, tempFilteredValArray);
			if (analysisConfig->postCFDFilter)
				processSuccess&=lowPassFilter(tempFilteredValArray, NUM_DIGITIZER_SAMPLES,analysisConfig->postCFDFactor);

			if (outputSample)
			{
				sample->numSamples=NUM_DIGITIZER_SAMPLES;
				sample->MSPS=config->sampleRateMSPS;
				sample->fValues[ch]=new float[NUM_DIGITIZER_SAMPLES];
				processSuccess&=clone(tempValArray, NUM_DIGITIZER_SAMPLES, sample->fValues[ch]);
				//processSuccess&=clone(tempFilteredValArray, NUM_DIGITIZER_SAMPLES, sample->fValues[ch]);
			}

			
			
            int positionOfThresholdCrossing;
			int positionOfCFDMin, positionOfCFDMax;
            float minCFDValue, maxCFDValue;

			processSuccess&=findMinMaxValue(tempFilteredValArray, NUM_DIGITIZER_SAMPLES, 0, NUM_DIGITIZER_SAMPLES, minCFDValue, positionOfCFDMin, maxCFDValue, positionOfCFDMax);
                
            processSuccess&=findIntersection(tempFilteredValArray, NUM_DIGITIZER_SAMPLES, positionOfCFDMin, positionOfCFDMax, 2, CFDThreshold, false, positionOfThresholdCrossing);
            float crossingPositionInterpolated;
			float cfdIndex;
            if (positionOfThresholdCrossing>=positionOfCFDMin)
            {
                float slope, offset;
                //linear fit over 4 points of slope
                processSuccess&=linearFit(tempFilteredValArray, NUM_DIGITIZER_SAMPLES, positionOfThresholdCrossing-2, positionOfThresholdCrossing+1, slope, offset);
				cfdIndex=(CFDThreshold-offset)/slope;
				int indexLow=(int)(cfdIndex);
				float d=cfdIndex-indexLow;
				float time=rawEvent.tValues[indexLow]*(1-d)+(d)*rawEvent.tValues[indexLow+1];
				if (analysisConfig->useTimeCFD)
					stats->channelStatistics[ch].timeOfCFDCrossing=time;
				else
					stats->channelStatistics[ch].timeOfCFDCrossing=cfdIndex/(config->sampleRateMSPS/1000.0);

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

			processSuccess&=findMinMaxValue(tempValArray, NUM_DIGITIZER_SAMPLES, 0, NUM_DIGITIZER_SAMPLES, stats->channelStatistics[ch].minValue, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].maxValue, stats->channelStatistics[ch].indexOfMax);
			
			if (stats->channelStatistics[ch].indexOfMin>0 && stats->channelStatistics[ch].indexOfMin<NUM_DIGITIZER_SAMPLES)
				stats->channelStatistics[ch].timeOfMin=rawEvent.tValues[stats->channelStatistics[ch].indexOfMin];

			if (stats->channelStatistics[ch].indexOfMax>0 && stats->channelStatistics[ch].indexOfMax<NUM_DIGITIZER_SAMPLES)
				stats->channelStatistics[ch].timeOfMax=rawEvent.tValues[stats->channelStatistics[ch].indexOfMax];

			processSuccess&=findHalfRise(tempValArray, NUM_DIGITIZER_SAMPLES, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].baseline, stats->channelStatistics[ch].indexOfHalfRise);
			if (stats->channelStatistics[ch].indexOfHalfRise>0 && stats->channelStatistics[ch].indexOfHalfRise<NUM_DIGITIZER_SAMPLES)
				stats->channelStatistics[ch].timeOfHalfRise=rawEvent.tValues[stats->channelStatistics[ch].indexOfHalfRise];

			int samplesPerMicrosecond=config->sampleRateMSPS;
			int startOffset=(analysisConfig->startGate*samplesPerMicrosecond)/1000;
            int shortGateOffset=(analysisConfig->shortGate*samplesPerMicrosecond)/1000;
            int longGateOffset=(analysisConfig->longGate*samplesPerMicrosecond)/1000;
			int timeOffset;
			switch (analysisConfig->timeOffset)
			{
			case CFD_TIME:
				timeOffset=(int)cfdIndex;
				break;
			case TIME_HALF_RISE:
				timeOffset=stats->channelStatistics[ch].indexOfHalfRise;
				break;
			default:
				timeOffset=stats->channelStatistics[ch].indexOfMin;
				break;
			}

			if (outputSample)
			{

				sample->baseline=stats->channelStatistics[ch].baseline;
				if (timeOffset+startOffset>=0 && timeOffset+startOffset<=NUM_DIGITIZER_SAMPLES)
				sample->indexStart=timeOffset+startOffset;
				if (timeOffset+shortGateOffset>=0 && timeOffset+shortGateOffset<=NUM_DIGITIZER_SAMPLES)
				sample->indexShortEnd=timeOffset+shortGateOffset;
				if (timeOffset+longGateOffset>=0 && timeOffset+longGateOffset<=NUM_DIGITIZER_SAMPLES)
				sample->indexLongEnd=timeOffset+longGateOffset;
				if (timeOffset>=0 && timeOffset<=NUM_DIGITIZER_SAMPLES)
					sample->cfdTime=timeOffset;
			}

			processSuccess&=calculateIntegrals(tempValArray, NUM_DIGITIZER_SAMPLES, stats->channelStatistics[ch].baseline, timeOffset+startOffset, timeOffset+shortGateOffset, timeOffset+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
            processSuccess&=calculatePSD(stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral, stats->channelStatistics[ch].PSD);

			stats->channelStatistics[ch].secondsFromFirstEvent=getSecondsFromFirstEvent(firstEventTimestamp, *rawEvent.timestamp);
		}
		//write -1 to channel number => no data
		else
			stats->channelStatistics[ch].channelNumber=-1;
	}
	if (outputSample)
	{
		sample->stats=*stats;
		emit newEventSample(sample);
	}
	if (processedEvents)
		processedEvents->push_back(stats);
}

void DRSAcquisitionThread::stopAcquisition()
{
	acquiring=false;
	if (processedEvents && processedEvents->size()>0)
		emit newProcessedEvents(processedEvents);
	
	emit eventAcquisitionFinished();
	
}