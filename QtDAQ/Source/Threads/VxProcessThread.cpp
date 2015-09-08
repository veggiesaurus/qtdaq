#include <QMessageBox>
#include <QDebug>
#include "Threads/VxProcessThread.h"

VxProcessThread::VxProcessThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, int s_bufferLength, QObject *parent)
	: QThread(parent)
{
	eventSize = 0;
	file = NULL;
	textStream = NULL;
	dataStream = NULL;

	sampleNextEvent = true;
	outputCurrentChannel = false;
	outputCurrentChannelUncorrected = false;
	numSamples = 1024;
	tempValArray = new float[numSamples];
	tempFilteredValArray = new float[numSamples];
	tempMedianArray = new float[numSamples];
	memset(tempValArray, 0, sizeof(float)*numSamples);
	memset(tempFilteredValArray, 0, sizeof(float)*numSamples);
	processedEvents = new QVector < EventStatistics* > ;
	processedEvents->setSharable(true);

    //buffers
	rawBuffers[0] = s_rawBuffer1;
	rawBuffers[1] = s_rawBuffer2;
	rawMutexes[0] = s_rawBuffer1Mutex;
	rawMutexes[1] = s_rawBuffer2Mutex;
	bufferLength = s_bufferLength;
}

VxProcessThread::~VxProcessThread()
{
	processingMutex.lock();
	if (file)
	{
		file->close();
		SAFE_DELETE(file);
	}
	processedEventsMutex.lock();
	if (processedEvents)
	{
		for (auto& i : *processedEvents)
		{
			SAFE_DELETE(i);
		}
		processedEvents->clear();
	}
	SAFE_DELETE(processedEvents);
	processedEventsMutex.unlock();

	SAFE_DELETE(tempFilteredValArray);
	SAFE_DELETE(tempValArray);
	SAFE_DELETE(tempMedianArray);
	processingMutex.unlock();

}

bool VxProcessThread::initVxProcessThread(AnalysisConfig* s_analysisConfig, int updateTime)
{
	if (!processedEvents)
	{
		processedEvents = new QVector < EventStatistics* > ;
		processedEvents->setSharable(true);
	}
	processedEvents->clear();
	eventSize = 0;
	analysisConfig = s_analysisConfig;

	updateTimer.stop();

	updateTimer.setInterval(updateTime);
	connect(&updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerTimeout()));
	updateTimer.start();
	return true;
}

void VxProcessThread::restartProcessThread()
{
	processedEventsMutex.lock();
	if (processedEvents && processedEvents->size())
	{
		for (int i = 0; i < processedEvents->size(); i++)
		{
			SAFE_DELETE((*processedEvents)[i]);
		}
		processedEvents->clear();
	}
	processedEventsMutex.unlock();
	initVxProcessThread(analysisConfig, updateTimer.interval());
}

void VxProcessThread::onResumeProcessing()
{
	currentBufferIndex = 0;
	currentBufferPosition = 0;

	while (true)
	{
		rawMutexes[currentBufferIndex]->lock();
		for (int i = 0; i < bufferLength; i++)
		{
			//break when encountering first processed event
			if (rawBuffers[currentBufferIndex][i].processed)
				break;
			processEvent(&(rawBuffers[currentBufferIndex][i]), sampleNextEvent);
		}
		rawMutexes[currentBufferIndex]->unlock();
		currentBufferIndex = 1 - currentBufferIndex;
	}
}

void VxProcessThread::resetTriggerTimerAndV8()
{
	previousRawTriggerTag = 0;
	wraparoundCounter = 0;
	eventCounter = 0;

	if (analysisConfig->bInitialV8)
        runV8CodeInitial();
}

void VxProcessThread::run()
{
    compileV8();

    resetTriggerTimerAndV8();

	currentBufferIndex = 0;
	currentBufferPosition = 0;
	int eventCounter = 0;
	int bufferCounter = 0;
	while (true)
	{
		rawMutexes[currentBufferIndex]->lock();
		for (int i = 0; i < bufferLength; i++)
		{
			EventVx& ev = rawBuffers[currentBufferIndex][i];
			if (ev.processed)
				return;
			processEvent(&ev, sampleNextEvent);
			ev.processed = true;
			eventCounter++;
		}
		rawMutexes[currentBufferIndex]->unlock();
		currentBufferIndex = 1 - currentBufferIndex;
		bufferCounter++;
	}
}

void VxProcessThread::readFilterFile(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	if (args.Length())
	{
		String::Utf8Value fileName(args[0]->ToString());
		if (fileName.length())
		{
			VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
			QFile* file = new QFile(*fileName);
			if (!file->open(QIODevice::ReadOnly))
				return;
			QTextStream* textStream = new QTextStream(file);
			thread->analysisConfig->filter.clear();
			while (!textStream->atEnd())
			{
				float val;
				(*textStream) >> val;
				thread->analysisConfig->filter.push_back(val);
			}
			if (thread->analysisConfig->filter.size())
				thread->analysisConfig->useOptimalFilter = true;

			file->close();
			SAFE_DELETE(textStream);
			SAFE_DELETE(file);
		}
	}
}

void VxProcessThread::setDownsampleFactor(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	int downSample = args[0]->ToInt32()->Int32Value();
	if (downSample > 1 && downSample < 30)
		thread->analysisConfig->samplingReductionFactor = downSample;
}

void VxProcessThread::printSampleToStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	thread->outputCurrentChannel = true;
}

void VxProcessThread::printUncorrectedSampleToStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	thread->outputCurrentChannelUncorrected = true;
}

void VxProcessThread::openFileStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	if (args.Length())
	{
		String::Utf8Value fileName(args[0]->ToString());
		if (fileName.length())
		{
			VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
			if (thread->file)
			{
				thread->file->close();
				SAFE_DELETE(thread->file);
			}
			thread->file = new QFile(*fileName);
			if (!thread->file->open(QIODevice::WriteOnly))
				return;
			thread->textStream = new QTextStream(thread->file);
		}
	}
}

void VxProcessThread::closeStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	if (thread->file)
	{
		if (thread->textStream)
			thread->textStream->flush();

		thread->file->close();

		SAFE_DELETE(thread->textStream);
		SAFE_DELETE(thread->dataStream);
		SAFE_DELETE(thread->file);
	}
}

void VxProcessThread::openBinaryFileStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	if (args.Length())
	{
		String::Utf8Value fileName(args[0]->ToString());
		if (fileName.length())
		{
			VxProcessThread* thread = reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
			thread->file = new QFile(*fileName);
			if (!thread->file->open(QIODevice::WriteOnly))
				return;
			thread->dataStream = new QDataStream(thread->file);
		}
	}
}

void VxProcessThread::onNewRawEvents(QVector<EventVx*>* events)
{
	for (int i = 0; i < events->size(); i++)
	{
		processEvent((*events)[i], sampleNextEvent);
		freeVxEvent((*events)[i]);
	}
	events->clear();
	SAFE_DELETE(events);
}


void VxProcessThread::onUpdateTimerTimeout()
{
	sampleNextEvent = true;
	processedEventsMutex.lock();
	if (processedEvents && processedEvents->size() > 0)
	{
		emit newProcessedEvents(processedEvents);
		processedEvents = new QVector<EventStatistics*>();
		processedEvents->setSharable(true);
	}
	processedEventsMutex.unlock();
}

void VxProcessThread::onAnalysisConfigChanged()
{
	v8Mutex.lock();
	v8RecompileRequired = true;
	v8Mutex.unlock();
}

void VxProcessThread::processEvent(EventVx* rawEvent, bool outputSample)
{
	v8Mutex.lock();
	if (v8RecompileRequired)
	{
		v8Mutex.unlock();
		compileV8();
	}
	else
		v8Mutex.unlock();

	processingMutex.lock();
	if (analysisConfig->bPreAnalysisV8)        
        //runV8CodeInitial();
        runV8CodePreAnalysis();
	EventStatistics* stats = new EventStatistics();
	stats->timestamp.millisecond = rawEvent->info.TriggerTimeTag;
	stats->runIndex = rawEvent->runIndex;
	bool vx1742Mode = false;
	if (rawEvent->info.BoardId == 2)
		vx1742Mode = true;

	//timing

	//check for wraparound (once every 35s?)
	rawEvent->info.TriggerTimeTag %= INT32_MAX;
	if (rawEvent->info.TriggerTimeTag <= previousRawTriggerTag)
		wraparoundCounter++;
	uint64_t adjustedTime = (uint64_t)wraparoundCounter*(uint64_t)INT32_MAX + (uint64_t)rawEvent->info.TriggerTimeTag;
	//converting from counter to time in millis (trigger time tag has 8ns LSB)
	double timeMillis = adjustedTime*(8.0e-9)*1e3;
	//if this is the start event
	if (wraparoundCounter == 0 && previousRawTriggerTag == 0)
		startTimeMillis = timeMillis;
	//adjust to start event time
	timeMillis -= startTimeMillis;
	stats->triggerTimeAdjustedMillis = timeMillis;
	eventCounter++;
	previousRawTriggerTag = rawEvent->info.TriggerTimeTag;

	//temperature
	int temperatureIndex = (int)(timeMillis / 5000);
	float currentTemperature = 0;
	if (temperatureIndex >= 0 && temperatureIndex < temperatureArray.size())
		currentTemperature = temperatureArray[temperatureIndex];

	stats->serial = rawEvent->info.BoardId;
	bool processSuccess = true;
	EventSampleData* sample = nullptr;
	if (outputSample)
	{
		sample = new EventSampleData();
		memset(sample, 0, sizeof(EventSampleData));
	}
	//4GSPS for Vx1761
	float GSPS = 4.0 / analysisConfig->samplingReductionFactor;
	//1GSPS for Vx1742
	if (vx1742Mode)
		GSPS = 1.0 / analysisConfig->samplingReductionFactor;
	bool foundNumSamples = false;

	int primaryCFDChannel = -1;

	float cfdOverrideTime = -1;
	if (primaryCFDChannel >= 0 && primaryCFDChannel < NUM_DIGITIZER_CHANNELS)
	{
		processSuccess = processChannel(vx1742Mode, rawEvent, primaryCFDChannel, stats, GSPS, sample);
		cfdOverrideTime = stats->channelStatistics[primaryCFDChannel].timeOfCFDCrossing;
	}
	for (int ch = 0; ch < NUM_DIGITIZER_CHANNELS; ch++)
	{
		stats->channelStatistics[ch].temperature = currentTemperature;
		if (ch != primaryCFDChannel)
			processSuccess = processChannel(vx1742Mode, rawEvent, ch, stats, GSPS, sample, cfdOverrideTime);
		stats->channelStatistics[ch].secondsFromFirstEvent = stats->triggerTimeAdjustedMillis / 1000.0f;
	}

	//Time of flight
	int chTOF = analysisConfig->stopPulseChannel;
	if (analysisConfig->useTimeOfFlight && rawEvent->data.ChSize[chTOF] && rawEvent->data.ChSize[chTOF] == numSamples && rawEvent->data.DataChannel[chTOF])
	{
		for (int i = 0; i < numSamples; i++)
			tempValArray[i] = ((float)rawEvent->data.DataChannel[chTOF][i]);

		if (outputSample)
		{
			sample->fValues[chTOF] = new float[numSamples];
			processSuccess &= clone(tempValArray, numSamples, sample->fValues[chTOF]);
		}
		for (int ch = 0; ch < NUM_DIGITIZER_CHANNELS; ch++)
		{
			SampleStatistics& statsCh = stats->channelStatistics[ch];
			//ignore empty and stop pulse channels
			if (!rawEvent->data.ChSize[ch] || ch == chTOF)
				continue;

			float startTime;

			switch (analysisConfig->timeOffsetPulse)
			{
			case CFD_TIME:
				startTime = statsCh.timeOfCFDCrossing + analysisConfig->startOffSetPulse;
				break;
			case TIME_HALF_RISE:
				startTime = statsCh.timeOfHalfRise + analysisConfig->startOffSetPulse;
				break;
			default:
				startTime = statsCh.timeOfMin + analysisConfig->startOffSetPulse;
				break;
			}

			int startIndex = startTime*GSPS;

			if (outputSample)
				sample->indexOfTOFStartPulse = startIndex;

			int positionOfPulseThresholdCrossing;
			processSuccess &= findIntersection(tempValArray, numSamples, startIndex, numSamples - 1, 2, analysisConfig->stopPulseThreshold, true, positionOfPulseThresholdCrossing);
			float pulseCrossingPositionInterpolated;
			float indexTOF = -1;
			float stopTime = positionOfPulseThresholdCrossing / GSPS;

			if (positionOfPulseThresholdCrossing >= startIndex)
			{
				float slope, offset;
				//linear fit over 4 points of slope
				processSuccess &= linearFit(tempValArray, numSamples, positionOfPulseThresholdCrossing - 8, positionOfPulseThresholdCrossing + 7, slope, offset);
				indexTOF = (analysisConfig->stopPulseThreshold - offset) / slope;
				int indexLow = (int)(indexTOF);
				float d = indexTOF - indexLow;
				stopTime = (indexLow / GSPS)*(1 - d) + (d)*((indexLow + 1) / GSPS);
			}
			//successful TOF calculation

			if (stopTime > 0)
			{
				statsCh.timeOfFlight = stopTime - startTime;
				if (outputSample)
				{
					sample->indexOfTOFEndPulse = stopTime*GSPS;
				}
			}
			else
			{
				statsCh.timeOfFlight = -1;
				if (outputSample)
				{
					sample->indexOfTOFEndPulse = stopTime*GSPS;
				}
			}
		}
	}

	if (analysisConfig->bPostEventV8)
		runV8CodePostEventAnalysis(stats);

	if (outputSample)
	{
		sample->stats = *stats;
		emit newEventSample(sample);
		sampleNextEvent = false;
	}
	processedEventsMutex.lock();
	processedEvents->push_back(stats);
	processedEventsMutex.unlock();
	//delete stats;
	processingMutex.unlock();

}

bool VxProcessThread::processChannel(bool vx1742Mode, EventVx* rawEvent, int ch, EventStatistics* stats, float GSPS, EventSampleData* sample, float cfdOverrideTime)
{
	bool processSuccess = true;
	int chSize;
	if (vx1742Mode)
		chSize = rawEvent->fData.ChSize[ch];
	else
		chSize = rawEvent->data.ChSize[ch];
	//ignore empty and stop pulse channels
	if (((chSize && rawEvent->data.DataChannel[ch]) || (chSize && rawEvent->fData.DataChannel[ch])) && !(analysisConfig->useTimeOfFlight && ch == analysisConfig->stopPulseChannel))
	{

		//change of sample num (should only happen with first event)
		if (chSize / analysisConfig->samplingReductionFactor != numSamples)
		{
			//sometimes ch1 has two extra entries, which are zero. Account for this special case by ignoring changes by +2
			if (chSize == numSamples + 2)
				return false;
			numSamples = chSize / analysisConfig->samplingReductionFactor;
			SAFE_DELETE_ARRAY(tempValArray);
			SAFE_DELETE_ARRAY(tempFilteredValArray);
			SAFE_DELETE_ARRAY(tempMedianArray);
			tempValArray = new float[numSamples];
			tempFilteredValArray = new float[numSamples];
			tempMedianArray = new float[numSamples];
			memset(tempValArray, 0, sizeof(float)*numSamples);
			memset(tempFilteredValArray, 0, sizeof(float)*numSamples);
			memset(tempMedianArray, 0, sizeof(float)*numSamples);
		}
		stats->channelStatistics[ch].channelNumber = ch;
		if (analysisConfig->bitsDropped)
		{
			int bitdropFactor = 1 << analysisConfig->bitsDropped;

			if (vx1742Mode)
			{
				for (int i = 0; i < numSamples; i++)
				{
					int index = (i)*analysisConfig->samplingReductionFactor;
					tempValArray[i] = 0.25f*bitdropFactor*((int)rawEvent->fData.DataChannel[ch][index] / bitdropFactor);
				}
			}
			else
			{
				for (int i = 0; i < numSamples; i++)
				{
					int index = (i)*analysisConfig->samplingReductionFactor;
					tempValArray[i] = bitdropFactor*((int)rawEvent->data.DataChannel[ch][index] / bitdropFactor);
				}
			}
		}
		else
		{
			if (vx1742Mode)
			{
				for (int i = 0; i < numSamples; i++)
				{
					int index = (i)*analysisConfig->samplingReductionFactor;
					tempValArray[i] = 0.25f*rawEvent->fData.DataChannel[ch][index];
				}
			}
			else
			{
				for (int i = 0; i < numSamples; i++)
				{
					int index = (i)*analysisConfig->samplingReductionFactor;
					tempValArray[i] = ((float)rawEvent->data.DataChannel[ch][index]);
				}
			}
		}

		//medianfilter(tempValArray, tempMedianArray, numSamples);
		//processSuccess &= findBaseline(tempMedianArray, 0, analysisConfig->baselineSampleRange, analysisConfig->baselineSampleSize, stats->channelStatistics[ch].baseline);
		processSuccess &= findBaseline(tempValArray, 0, analysisConfig->baselineSampleRange, analysisConfig->baselineSampleSize, stats->channelStatistics[ch].baseline);

		if (analysisConfig->preCFDFilter)
			processSuccess &= lowPassFilter(tempValArray, numSamples, analysisConfig->preCFDFactor);
		//if the digital gain is not unity
		if (abs(analysisConfig->digitalGain - 1.00f) > 0.01f)
		{
			processSuccess &= applyGain(tempValArray, numSamples, analysisConfig->digitalGain, stats->channelStatistics[ch].baseline);
			if (analysisConfig->digitalGain < 0)
				stats->channelStatistics[ch].baseline = 1024 - stats->channelStatistics[ch].baseline;
		}
		int thresholdCrossing;
		float threshold = 990.0f;
		processSuccess &= findIntersection(tempValArray, numSamples, 0, numSamples, 4, threshold, true, thresholdCrossing);


		processSuccess &= clone(tempValArray, numSamples, tempFilteredValArray);
		float CFDThreshold = 512.0f;
		processSuccess &= cfdSampleOptimized(tempValArray, stats->channelStatistics[ch].baseline, numSamples, analysisConfig->CFDFraction, analysisConfig->CFDLength, analysisConfig->CFDOffset, CFDThreshold, 1.f / GSPS, tempFilteredValArray);
		if (analysisConfig->postCFDFilter)
			processSuccess &= lowPassFilter(tempFilteredValArray, numSamples, analysisConfig->postCFDFactor);

		int positionOfThresholdCrossing;
		int positionOfCFDMin, positionOfCFDMax;
		float minCFDValue, maxCFDValue;

		processSuccess &= findMinMaxValue(tempFilteredValArray, numSamples, 0, numSamples, minCFDValue, positionOfCFDMin, maxCFDValue, positionOfCFDMax);

		processSuccess &= findIntersection(tempFilteredValArray, numSamples, positionOfCFDMin, positionOfCFDMax, 2, CFDThreshold, false, positionOfThresholdCrossing);
		float cfdCrossing = 0;
		if (positionOfThresholdCrossing >= positionOfCFDMin)
		{
			float slope, offset;
			//linear fit over 4 points of slope
			processSuccess &= linearFit(tempFilteredValArray, numSamples, positionOfThresholdCrossing - 2, positionOfThresholdCrossing + 1, slope, offset);
			cfdCrossing = (CFDThreshold - offset) / slope;
			int indexLow = (int)(cfdCrossing);
			float d = cfdCrossing - indexLow;

			float time = (indexLow / GSPS)*(1 - d) + (d)*((indexLow + 1) / GSPS);
			if (analysisConfig->useTimeCFD)
				stats->channelStatistics[ch].timeOfCFDCrossing = time;
			else
				stats->channelStatistics[ch].timeOfCFDCrossing = cfdCrossing / GSPS;

			//if there is a previous channel
			if (ch > 0 && stats->channelStatistics[ch - 1].channelNumber != -1 && stats->channelStatistics[ch - 1].timeOfCFDCrossing > -1000)
			{
				stats->channelStatistics[ch].deltaTprevChannelCFD = stats->channelStatistics[ch].timeOfCFDCrossing - stats->channelStatistics[ch - 1].timeOfCFDCrossing;
			}
			else
				//dummy deltaT
				stats->channelStatistics[ch].deltaTprevChannelCFD = -1000;
		}
		else
		{
			stats->channelStatistics[ch].timeOfCFDCrossing = -1000;

		}

		processSuccess &= findMinMaxValue(tempValArray, numSamples, 0, numSamples, stats->channelStatistics[ch].minValue, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].maxValue, stats->channelStatistics[ch].indexOfMax);

		if (stats->channelStatistics[ch].indexOfMin > 0 && stats->channelStatistics[ch].indexOfMin < numSamples)
			stats->channelStatistics[ch].timeOfMin = stats->channelStatistics[ch].indexOfMin / GSPS;

		if (stats->channelStatistics[ch].indexOfMax>0 && stats->channelStatistics[ch].indexOfMax < numSamples)
			stats->channelStatistics[ch].timeOfMax = stats->channelStatistics[ch].indexOfMax / GSPS;

		processSuccess &= findHalfRise(tempValArray, numSamples, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].baseline, stats->channelStatistics[ch].indexOfHalfRise);
		if (stats->channelStatistics[ch].indexOfHalfRise>0 && stats->channelStatistics[ch].indexOfHalfRise < numSamples)
			stats->channelStatistics[ch].timeOfHalfRise = stats->channelStatistics[ch].indexOfHalfRise / GSPS;

		//0.25 ns per sample
		int samplesPerMicrosecond = (int)(1000 * GSPS);
		int startOffset = (analysisConfig->startGate*samplesPerMicrosecond) / 1000;
		int shortGateOffset = (analysisConfig->shortGate*samplesPerMicrosecond) / 1000;
		int longGateOffset = (analysisConfig->longGate*samplesPerMicrosecond) / 1000;
		int timeOffset;
		switch (analysisConfig->timeOffset)
		{
		case CFD_TIME:
			timeOffset = (int)cfdCrossing;
			break;
		case TIME_HALF_RISE:
			timeOffset = stats->channelStatistics[ch].indexOfHalfRise;
			break;
		default:
			timeOffset = stats->channelStatistics[ch].indexOfMin;
			break;
		}

		//temp hack
		//timeOffset=thresholdCrossing;
		if (cfdOverrideTime >= 0)
			cfdCrossing = cfdOverrideTime*GSPS;
		//Charge comparison
		//processSuccess &= calculateIntegralsLinearBaseline(tempValArray, numSamples, analysisConfig->baselineSampleSize, timeOffset + startOffset, timeOffset + shortGateOffset, timeOffset + longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
		
		//processSuccess &= calculateIntegrals(tempValArray, numSamples, stats->channelStatistics[ch].baseline, timeOffset + startOffset, timeOffset + shortGateOffset, timeOffset + longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
		processSuccess &= calculateIntegralsCorrected(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdCrossing + startOffset, cfdCrossing + shortGateOffset, cfdCrossing + longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
		//processSuccess&=calculateIntegralsTrapezoidal(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdIndex+startOffset, cfdIndex+shortGateOffset, cfdIndex+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
		//processSuccess&=calculateIntegralsSimpson(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdIndex+startOffset, cfdIndex+shortGateOffset, cfdIndex+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
		stats->channelStatistics[ch].shortGateIntegral *= analysisConfig->samplingReductionFactor;
		stats->channelStatistics[ch].longGateIntegral *= analysisConfig->samplingReductionFactor;
		processSuccess &= calculatePSD(stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral, stats->channelStatistics[ch].PSD);

		//temperature corrections (only if defined)
		if (analysisConfig->useTempCorrection && stats->channelStatistics[ch].temperature>0.01)
		{
			//float temperatureFactor = 1.00 + (stats->channelStatistics[ch].temperature - analysisConfig->referenceTemperature)* (-analysisConfig->scalingVariation / 100);
			float temperatureFactor = 1.00 / (1.00 + (stats->channelStatistics[ch].temperature - analysisConfig->referenceTemperature)*(analysisConfig->scalingVariation / 100)); 

			stats->channelStatistics[ch].shortGateIntegral *= temperatureFactor;
			stats->channelStatistics[ch].longGateIntegral *= temperatureFactor;
		}


		//Linear filter
		if (analysisConfig->useOptimalFilter)
		{
			float S = 0;
			processSuccess &= linearFilter(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdCrossing + startOffset, analysisConfig->filter.data(), analysisConfig->filter.size(), S);
			if (stats->channelStatistics[ch].longGateIntegral > 0)
				stats->channelStatistics[ch].filteredPSD = 100 * S / stats->channelStatistics[ch].longGateIntegral;
		}

		//Zero crossing
		//gen
		//float ZCstart=0.152f;
		//float ZCstop=0.662f;
		//62mev
		float ZCstart = 0.095f;
		float ZCstop = 0.608f;

		float ZCint;
		processSuccess &= calculateZeroCrossing(tempValArray, numSamples, stats->channelStatistics[ch].baseline, timeOffset + startOffset, timeOffset + longGateOffset, stats->channelStatistics[ch].longGateIntegral, ZCstart, ZCstop, ZCint);
		stats->channelStatistics[ch].custom5 = ZCint;
		if (sample)
		{
			sample->numSamples = numSamples;
			sample->MSPS = GSPS * 1000;
			if (!sample->tValues)
			{
				sample->tValues = new float[numSamples];
				for (int i = 0; i < numSamples; i++)
					sample->tValues[i] = i / GSPS;
			}
			sample->fValues[ch] = new float[numSamples];
			//processSuccess&=clone(tempFilteredValArray, numSamples, sample->fValues[ch]);
			if (analysisConfig->displayCFDSignal)
				processSuccess &= clone(tempFilteredValArray, numSamples, sample->fValues[ch]);
			else
				processSuccess &= clone(tempValArray, numSamples, sample->fValues[ch]);

			sample->baseline = stats->channelStatistics[ch].baseline;
			sample->indexStart = timeOffset + startOffset;
			sample->indexShortEnd = timeOffset + shortGateOffset;
			sample->indexLongEnd = timeOffset + longGateOffset;
			sample->cfdTime = timeOffset / GSPS;
		}

		if (analysisConfig->bPostChannelV8)
			runV8CodePostChannelAnalysis(&(stats->channelStatistics[ch]));

		//do output if necessary
		if (outputCurrentChannel)
		{
			printTruncatedSamples(tempValArray, stats->channelStatistics[ch].baseline, timeOffset + startOffset, timeOffset + longGateOffset);
			outputCurrentChannel = false;
		}
		else if (outputCurrentChannelUncorrected)
		{
			printTruncatedSamples(tempValArray, 0, 0, numSamples - 1);
			outputCurrentChannelUncorrected = false;
		}
	}
	//write -1 to channel number => no data
	else
		stats->channelStatistics[ch].channelNumber = -1;
	return processSuccess;
}

void VxProcessThread::compileV8()
{
    //v8
    v8Mutex.lock();

    V8::InitializeICU();
    //plat = platform::CreateDefaultPlatform();
    //V8::InitializePlatform(plat);
    V8::Initialize();

    ArrayBufferAllocator allocator;
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = &allocator;
    bool hasException = false;
    isolate = Isolate::New(create_params);
    {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);

        Local<ObjectTemplate> global = ObjectTemplate::New();
        //functions

        global->Set(isolate, "printMessage", FunctionTemplate::New(isolate, printMessage));
        global->Set(isolate, "setCustomParameterName", FunctionTemplate::New(isolate, setCustomParameterName));
        global->Set(isolate, "printSampleToStream", FunctionTemplate::New(isolate, printSampleToStream, External::New(isolate, this)));
        global->Set(isolate, "printUncorrectedSampleToStream", FunctionTemplate::New(isolate, printUncorrectedSampleToStream, External::New(isolate, this)));
        global->Set(isolate, "readFilterFile", FunctionTemplate::New(isolate, readFilterFile, External::New(isolate, this)));
        global->Set(isolate, "openFileStream", FunctionTemplate::New(isolate, openFileStream, External::New(isolate, this)));
        global->Set(isolate, "closeStream", FunctionTemplate::New(isolate, closeStream, External::New(isolate, this)));
        global->Set(isolate, "openBinaryFileStream", FunctionTemplate::New(isolate, openBinaryFileStream, External::New(isolate, this)));
        global->Set(isolate, "setDownsampleFactor", FunctionTemplate::New(isolate, setDownsampleFactor, External::New(isolate, this)));

        globalTemplate.Reset(isolate, global);

        Local<Context> context = Context::New(isolate, NULL, global);
        persContext.Reset(isolate, context);

        Context::Scope context_scope(context);

        v8::TryCatch try_catch;
        try_catch.SetVerbose(true);

        Local<String> sourceInitial = String::NewFromUtf8(isolate, analysisConfig->customCodeInitial.toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
        Local<Script> localScriptInitial = Script::Compile(context, sourceInitial).ToLocalChecked();
        if (localScriptInitial.IsEmpty())
            checkV8Exceptions(try_catch, "Initialization");
        scriptInitial.Reset(isolate, localScriptInitial);

        Local<String> sourceDef = String::NewFromUtf8(isolate, analysisConfig->customCodeDef.toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
        Local<Script> localScriptDef = Script::Compile(context, sourceDef).ToLocalChecked();
        if (localScriptDef.IsEmpty())
            checkV8Exceptions(try_catch, "Initialization");
        scriptDef.Reset(isolate, localScriptDef);

        Local<String> sourcePre = String::NewFromUtf8(isolate, analysisConfig->customCodePreAnalysis.toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
        Local<Script> localScriptPre = Script::Compile(context, sourcePre).ToLocalChecked();
        if (localScriptPre.IsEmpty())
            checkV8Exceptions(try_catch, "Initialization");
        scriptPre.Reset(isolate, localScriptPre);

        Local<String> sourcePostChannel = String::NewFromUtf8(isolate, analysisConfig->customCodePostChannel.toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
        Local<Script> localScriptPostChannel = Script::Compile(context, sourcePostChannel).ToLocalChecked();
        if (localScriptPostChannel.IsEmpty())
            checkV8Exceptions(try_catch, "Post-analysis (per channel)");
        scriptPostChannel.Reset(isolate, localScriptPostChannel);

        QString prependedPostEventCode = "var chStats = [chStats0, chStats1, chStats2, chStats3];" + analysisConfig->customCodePostEvent;
        Local<String> sourcePostEvent = String::NewFromUtf8(isolate, prependedPostEventCode.toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
        Local<Script> localScriptPostEvent = Script::Compile(context, sourcePostEvent).ToLocalChecked();
        if (localScriptPostEvent.IsEmpty())
            checkV8Exceptions(try_catch, "Post-analysis (per event)");
        scriptPostEvent.Reset(isolate, localScriptPostEvent);


        Local<String> sourceFinished = String::NewFromUtf8(isolate, analysisConfig->customCodeFinal.toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
        Local<Script> localScriptFinished = Script::Compile(context, sourceFinished).ToLocalChecked();
        if (localScriptPostChannel.IsEmpty())
            checkV8Exceptions(try_catch, "After reading");
        scriptFinished.Reset(isolate, localScriptFinished);

       /* Handle<String> sourcePre = String::NewFromUtf8(isolate, analysisConfig->customCodePreAnalysis.toStdString().c_str());
        Handle<Script> handleScriptPre = Script::Compile(sourcePre);
        if (handleScriptPre.IsEmpty())
            checkV8Exceptions(try_catch, "Pre-analysis");
        scriptPre.Reset(isolate, handleScriptPre);

        Handle<String> sourcePostChannel = String::NewFromUtf8(isolate, analysisConfig->customCodePostChannel.toStdString().c_str());
        Handle<Script> handleScriptPostChannel = Script::Compile(sourcePostChannel);
        if (handleScriptPostChannel.IsEmpty())
            checkV8Exceptions(try_catch, "Post-analysis (per channel)");
        hasException = try_catch.HasCaught();
        scriptPostChannel.Reset(isolate, handleScriptPostChannel);

        //allow v8 code to access each channel stats via an array (channels 0-3)
        QString prependedPostEventCode = "var chStats = [chStats0, chStats1, chStats2, chStats3];" + analysisConfig->customCodePostEvent;
        Handle<String> sourcePostEvent = String::NewFromUtf8(isolate, prependedPostEventCode.toStdString().c_str());
        Handle<Script> handleScriptPostEvent = Script::Compile(sourcePostEvent);
        if (handleScriptPostEvent.IsEmpty())
            checkV8Exceptions(try_catch, "Post-analysis (per event)");
        scriptPostEvent.Reset(isolate, handleScriptPostEvent);

        Handle<String> sourceDef = String::NewFromUtf8(isolate, analysisConfig->customCodeDef.toStdString().c_str());
        Handle<Script> handleScriptDef = Script::Compile(sourceDef);
        if (handleScriptDef.IsEmpty())
            checkV8Exceptions(try_catch, "Definitions");
        scriptDef.Reset(isolate, handleScriptDef);

        Handle<String> sourceFinished = String::NewFromUtf8(isolate, analysisConfig->customCodeFinal.toStdString().c_str());
        Handle<Script> handleScriptFinished = Script::Compile(sourceFinished);
        if (handleScriptFinished.IsEmpty())
            checkV8Exceptions(try_catch, "After reading");
        scriptFinished.Reset(isolate, handleScriptFinished);*/

        Handle<ObjectTemplate> sampleStatsTemplate = GetSampleStatsTemplate(isolate);
        for (int i = 0; i < NUM_DIGITIZER_CHANNELS; i++)
        {
            Local<Object> localSampleStats = sampleStatsTemplate->NewInstance();
            allChannelsStatsObject[i].Reset(isolate, localSampleStats);
            context->Global()->Set(String::NewFromUtf8(isolate, "chStats" + QString::number(i).toLatin1()), localSampleStats);
        }

        Local<Object> localSampleStats = sampleStatsTemplate->NewInstance();
        sampleStatsObject.Reset(isolate, localSampleStats);
        context->Global()->Set(String::NewFromUtf8(isolate, "currentChStats"), localSampleStats);

        Handle<ObjectTemplate> eventStatsTemplate = GetEventStatsTemplate(isolate);
        Local<Object> localEventStats = eventStatsTemplate->NewInstance();
        eventStatsObject.Reset(isolate, localEventStats);
        context->Global()->Set(String::NewFromUtf8(isolate, "evStats"), localEventStats);
    }
	v8Mutex.unlock();
	v8RecompileRequired = false;

	if (analysisConfig->bDefV8)
        runV8CodeDefinitions();
}

bool VxProcessThread::runV8CodeDefinitions()
{
    v8Mutex.lock();
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
    //Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context = Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript = Local<Script>::New(isolate, scriptDef);
    Context::Scope context_scope(context);
    handleScript->Run();
	v8Mutex.unlock();
	return true;
}

bool VxProcessThread::runV8CodeInitial()
{
    v8Mutex.lock();    
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    //Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context = Local<Context>::New(isolate, persContext);

    Handle<Script> handleScript = Local<Script>::New(isolate, scriptInitial);
    Context::Scope context_scope(context);

    handleScript->Run();
	v8Mutex.unlock();
	return true;
}

bool VxProcessThread::runV8CodePreAnalysis()
{
    v8Mutex.lock();

	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);


    Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
    Handle<Context> context = Local<Context>::New(isolate, persContext);
    Handle<Script> handleScript = Local<Script>::New(isolate, scriptPre);
	Context::Scope context_scope(context);       
    v8::TryCatch try_catch;
    try_catch.SetVerbose(true);

    //Handle<String> source1 = String::NewFromUtf8(isolate, "var x = 12;");
    //Handle<Script> script1 = Script::Compile(source1);
    //if (script1.IsEmpty())
    //{
        //v8::Handle<v8::Message> message = try_catch.Message();
    //     v8::String::Utf8Value exception(try_catch.Exception());
    //    qDebug() << *exception;
    //}
    //script1->Run();

    handleScript->Run(context);
    if (try_catch.HasCaught())
    {
        v8::String::Utf8Value exception(try_catch.Exception());
        QString exceptionMessage = *exception;
        int lineNumber = 0;

        v8::Handle<v8::Message> message = try_catch.Message();
        if (message->GetLineNumber())
            lineNumber = message->GetLineNumber();
        qDebug() << "Error in line " + QString::number(lineNumber);
        qDebug() << exceptionMessage;
    }
	v8Mutex.unlock();
	return true;
}

bool VxProcessThread::runV8CodePostChannelAnalysis(SampleStatistics* chStats)
{
	v8Mutex.lock();
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context = Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript = Local<Script>::New(isolate, scriptPostChannel);
	Context::Scope context_scope(context);
	Local<Object> obj = Local<Object>::New(isolate, sampleStatsObject);
	obj->SetInternalField(0, External::New(isolate, chStats));
	handleScript->Run();
	v8Mutex.unlock();
	return true;
}
bool VxProcessThread::runV8CodePostEventAnalysis(EventStatistics* evStats)
{
	v8Mutex.lock();
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context = Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript = Local<Script>::New(isolate, scriptPostEvent);
	Context::Scope context_scope(context);

	Local<Object> obj = Local<Object>::New(isolate, eventStatsObject);
	obj->SetInternalField(0, External::New(isolate, evStats));

	Local<Object> objsCh[NUM_DIGITIZER_CHANNELS];

	for (int i = 0; i < NUM_DIGITIZER_CHANNELS; i++)
	{
		objsCh[i] = Local<Object>::New(isolate, allChannelsStatsObject[i]);
		objsCh[i]->SetInternalField(0, External::New(isolate, &evStats->channelStatistics[i]));
	}

	handleScript->Run();
	v8Mutex.unlock();
	return true;
}
bool VxProcessThread::runV8CodeFinal()
{
	v8Mutex.lock();
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	v8::Local<v8::ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Local<Context> context = Local<Context>::New(isolate, persContext);
	Local<Script> handleScriptFinished = Local<Script>::New(isolate, scriptFinished);
	Context::Scope context_scope(context);

	SampleStatistics* newSample = new SampleStatistics();
	newSample->custom5 = 999;
	Local<Object> obj = Local<Object>::New(isolate, sampleStatsObject);
	obj->SetInternalField(0, External::New(isolate, newSample));
	//sampleStatsObject.Reset(isolate, obj);

	handleScriptFinished->Run();
	v8Mutex.unlock();
	return true;
}


void VxProcessThread::printTruncatedSamples(float* sampleArray, float baseline, float startIndex, float endIndex)
{
	int numOutputSamples = (endIndex - startIndex + 1);
	if ((!textStream &&!dataStream) || !file || numOutputSamples < 1)
		return;
	bool binaryOutput = dataStream;
	if (binaryOutput)
		dataStream->writeRawData((const char*)&numOutputSamples, sizeof(int));
	if (binaryOutput)
	{
		for (int i = startIndex; i <= endIndex; i++)
		{
			float val = baseline - sampleArray[i];
			dataStream->writeRawData((const char*)&val, sizeof(float));
		}
	}
	else
	{
		for (int i = startIndex; i <= endIndex; i++)
		{
			float val = baseline - sampleArray[i];
			(*textStream) << val << " ";
			(*textStream) << endl;
		}
	}
}

void VxProcessThread::loadTemperatureLog(QString filename)
{
	QFile* file = new QFile(filename);
	if (!file->open(QIODevice::ReadOnly))
		return;
	temperatureArray.clear();
	QTextStream* textStream = new QTextStream(file);
	while (!textStream->atEnd())
	{
		float tempVal;
		(*textStream) >> tempVal;
		temperatureArray.push_back(tempVal);
	}

	file->close();
	SAFE_DELETE(textStream);
	SAFE_DELETE(file);
}
