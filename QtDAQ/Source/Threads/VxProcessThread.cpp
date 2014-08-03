#include "Threads/VxProcessThread.h"

VxProcessThread::VxProcessThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, QMutex* s_procBuffer1Mutex, QMutex* s_procBuffer2Mutex, EventStatistics* s_procBuffer1, EventStatistics* s_procBuffer2, QObject *parent)
	: QThread(parent)
{
	eventSize=0;
	updateTimer=NULL;
	file=NULL;
	textStream=NULL;
	dataStream=NULL;

	sampleNextEvent=true;
	outputCurrentChannel=false;
	outputCurrentChannelUncorrected=false;
	numSamples=1024;
	tempValArray=new float[numSamples];
	tempFilteredValArray=new float[numSamples];
	memset(tempValArray, 0, sizeof(float)*numSamples);
	memset(tempFilteredValArray, 0, sizeof(float)*numSamples);
	processedEvents=new QVector<EventStatistics*>;
	processedEvents->setSharable(true);
		
	//v8
	isolate=Isolate::GetCurrent();
	
	//buffers
	rawBuffers[0]=s_rawBuffer1;
	rawBuffers[1]=s_rawBuffer2;
	rawMutexes[0]=s_rawBuffer1Mutex;
	rawMutexes[1]=s_rawBuffer2Mutex;

	procBuffers[0]=s_procBuffer1;
	procBuffers[1]=s_procBuffer2;
	procMutexes[0]=s_procBuffer1Mutex;
	procMutexes[1]=s_procBuffer2Mutex;	
}



bool VxProcessThread::initVxProcessThread(AnalysisConfig* s_analysisConfig, int updateTime)
{		
	if (!processedEvents)
	{
		processedEvents=new QVector<EventStatistics*>;
		processedEvents->setSharable(true);
	}
	processedEvents->clear();
	eventSize=0;
	resetTriggerTimerAdjustments();
	analysisConfig=s_analysisConfig;
	if (updateTimer)
	{
		updateTimer->stop();
		updateTimer->disconnect();
		SAFE_DELETE(updateTimer);
	}

	
	compileV8();

	if (analysisConfig->bInitialV8)
	{
		runV8CodeInitial();
	}

	updateTimer=new QTimer();
	updateTimer->setInterval(updateTime);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerTimeout()));
    updateTimer->start();
	return true;
}

void VxProcessThread::restartProcessThread()
{
	if (processedEvents && processedEvents->size())
	{
		for (int i=0;i<processedEvents->size();i++)
		{
			SAFE_DELETE((*processedEvents)[i]);
		}
		processedEvents->clear();		
	}
	initVxProcessThread(analysisConfig, updateTimer->interval());	
}

void VxProcessThread::onResumeProcessing()
{
	currentBufferIndex=0;
	currentBufferPosition=0;
	
	while(true)
	{
		rawMutexes[currentBufferIndex]->lock();
		for (int i=0;i<EVENT_BUFFER_SIZE;i++)
		{			
			processEvent(&(rawBuffers[currentBufferIndex][i]), sampleNextEvent);
			sampleNextEvent=false;
		}
		rawMutexes[currentBufferIndex]->unlock();
		currentBufferIndex=1-currentBufferIndex;
	}
}

void VxProcessThread::resetTriggerTimerAdjustments()
{
	previousRawTriggerTag=0;
	wraparoundCounter=0;
	eventCounter=0;
}

void VxProcessThread::compileV8()
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
	
	Handle<Context> context = Context::New(isolate, NULL, global);
	persContext.Reset(isolate, context);
	globalTemplate.Reset(isolate, global);
	Context::Scope context_scope(context);

	Handle<String> sourceInitial = String::NewFromUtf8(isolate, analysisConfig->customCodeInitial.toStdString().c_str());
	Handle<Script> handleScriptInitial= Script::Compile(sourceInitial);
	scriptInitial.Reset(isolate, handleScriptInitial);

	Handle<String> sourcePre = String::NewFromUtf8(isolate, analysisConfig->customCodePreAnalysis.toStdString().c_str());
	Handle<Script> handleScriptPre= Script::Compile(sourcePre);
	scriptPre.Reset(isolate, handleScriptPre);
		
	Handle<String> sourcePostChannel = String::NewFromUtf8(isolate, analysisConfig->customCodePostChannel.toStdString().c_str());
	Handle<Script> handleScriptPostChannel= Script::Compile(sourcePostChannel);
	scriptPostChannel.Reset(isolate, handleScriptPostChannel);

	Handle<String> sourcePostEvent = String::NewFromUtf8(isolate, analysisConfig->customCodePostEvent.toStdString().c_str());
	Handle<Script> handleScriptPostEvent= Script::Compile(sourcePostEvent);
	scriptPostEvent.Reset(isolate, handleScriptPostEvent);
	
	Handle<String> sourceTimer = String::NewFromUtf8(isolate, analysisConfig->customCodeDef.toStdString().c_str());
	Handle<Script> handleScriptTimer= Script::Compile(sourceTimer);
	scriptDef.Reset(isolate, handleScriptTimer);

	Handle<String> sourceFinished = String::NewFromUtf8(isolate, analysisConfig->customCodeFinal.toStdString().c_str());
	Handle<Script> handleScriptFinished= Script::Compile(sourceFinished);
	scriptFinished.Reset(isolate, handleScriptFinished);

	Handle<ObjectTemplate> sampleStatsTemplate=GetSampleStatsTemplate(isolate);
	Local<Object> localSampleStats=sampleStatsTemplate->NewInstance();
	sampleStatsObject.Reset(isolate, localSampleStats);
	context->Global()->Set(String::NewFromUtf8(isolate, "chStats"), localSampleStats);

	Handle<ObjectTemplate> eventStatsTemplate=GetEventStatsTemplate(isolate);
	Local<Object> localEventStats=eventStatsTemplate->NewInstance();
	eventStatsObject.Reset(isolate, localEventStats);
	context->Global()->Set(String::NewFromUtf8(isolate, "evStats"), localEventStats);

	if (analysisConfig->bDefV8)
		runV8CodeDefinitions();
}

void VxProcessThread::run()
{	
	currentBufferIndex=0;
	currentBufferPosition=0;
	int eventCounter=0;
	int bufferCounter=0;
	while(true)
	{
		rawMutexes[currentBufferIndex]->lock();
		for (int i=0;i<EVENT_BUFFER_SIZE;i++)
		{
			EventVx& ev=rawBuffers[currentBufferIndex][i];
			if (ev.processed)
				return;			
			processEvent(&ev, sampleNextEvent);
			sampleNextEvent=false;
			ev.processed=true;
			eventCounter++;
		}
		rawMutexes[currentBufferIndex]->unlock();
		currentBufferIndex=1-currentBufferIndex;
		bufferCounter++;
	}
}

void VxProcessThread::readFilterFile(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	 if (args.Length())
	 {
		 String::Utf8Value fileName( args[0]->ToString() );
         if( fileName.length() ) 
         {
			 VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
			 QFile* file=new QFile(*fileName);
			 if (!file->open(QIODevice::ReadOnly))
				return;
			 QTextStream* textStream=new QTextStream(file);
			 thread->analysisConfig->filter.clear();
			 while (!textStream->atEnd())
			 {
				 float val;
				 (*textStream)>>val;
				 thread->analysisConfig->filter.push_back(val);
			 }
			 if (thread->analysisConfig->filter.size())
				 thread->analysisConfig->useOptimalFilter=true;
		 }
	 }
}

void VxProcessThread::setDownsampleFactor(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	int downSample=args[0]->ToInt32()->Int32Value();
	if (downSample>1 && downSample<30)
		thread->analysisConfig->samplingReductionFactor=downSample;
}

void VxProcessThread::printSampleToStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	thread->outputCurrentChannel=true;
}

void VxProcessThread::printUncorrectedSampleToStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
	thread->outputCurrentChannelUncorrected=true;
}

void VxProcessThread::openFileStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	 if (args.Length())
	 {
		 String::Utf8Value fileName( args[0]->ToString() );
         if( fileName.length() ) 
         {
			 VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
			 thread->file=new QFile(*fileName);
			 if (!thread->file->open(QIODevice::WriteOnly))
				return;
			 thread->textStream=new QTextStream(thread->file);
		 }
	 }
}

void VxProcessThread::closeStream(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);
	VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
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
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	 if (args.Length())
	 {
		 String::Utf8Value fileName( args[0]->ToString() );
         if( fileName.length() ) 
         {
			 VxProcessThread* thread=reinterpret_cast<VxProcessThread*>(v8::External::Cast(*args.Data())->Value());
			 thread->file=new QFile(*fileName);
			 if (!thread->file->open(QIODevice::WriteOnly))
				return;
			 thread->dataStream=new QDataStream(thread->file);
		 }
	 }
}

void VxProcessThread::onNewRawEvents(QVector<EventVx*>* events)
{
	for (int i=0;i<events->size();i++)
	{
		processEvent((*events)[i], sampleNextEvent);
		freeEvent((*events)[i]);
		sampleNextEvent=false;
	}
	events->clear();
	SAFE_DELETE(events);
}


void VxProcessThread::onUpdateTimerTimeout()
{	
	sampleNextEvent=true;
	if (processedEvents && processedEvents->size()>0)
	{        
		emit newProcessedEvents(processedEvents);
		processedEvents=new QVector<EventStatistics*>();
		processedEvents->setSharable(true);	
	}
}

void VxProcessThread::onAnalysisConfigChanged()
{
	compileV8();
}

void VxProcessThread::processEvent(EventVx* rawEvent, bool outputSample)
{
	if (analysisConfig->bPreAnalysisV8)
		runV8CodePreAnalysis();
	EventStatistics* stats=new EventStatistics();	
	stats->timestamp.millisecond=rawEvent->info.TriggerTimeTag;

	bool vx1742Mode=false;
	if (rawEvent->info.BoardId==2)
		vx1742Mode=true;

	//timing

	//check for wraparound (once every 35s?)
	if (rawEvent->info.TriggerTimeTag<=previousRawTriggerTag)
			wraparoundCounter++;
	uint64_t adjustedTime=(uint64_t)wraparoundCounter*(uint64_t)UINT32_MAX+(uint64_t)rawEvent->info.TriggerTimeTag;
	//converting from counter to time in millis (trigger time tag has 8ns LSB)
	double timeMillis=adjustedTime*(8.0e-9)*1e3;
	//if this is the start event
	if (wraparoundCounter==0 && previousRawTriggerTag ==0 )
		startTimeMillis=timeMillis;
	//adjust to start event time
	timeMillis-=startTimeMillis;
	stats->triggerTimeAdjustedMillis=timeMillis;
	eventCounter++;
	previousRawTriggerTag=rawEvent->info.TriggerTimeTag;
	

	stats->serial=rawEvent->info.BoardId;
	bool processSuccess=true;
	EventSampleData* sample;
	if (outputSample)
	{
		sample=new EventSampleData();		
		memset(sample, 0, sizeof(EventSampleData));		
	}
	//4GSPS for Vx1761
	float GSPS=4.0/analysisConfig->samplingReductionFactor;
	//1GSPS for Vx1742
	if (vx1742Mode)
		GSPS=1.0/analysisConfig->samplingReductionFactor;
	bool foundNumSamples=false;
	for (int ch=0;ch<NUM_DIGITIZER_CHANNELS;ch++)
	{
		int chSize;
		if (vx1742Mode)
			chSize=rawEvent->fData.ChSize[ch];
		else
			chSize=rawEvent->data.ChSize[ch];
		//ignore empty and stop pulse channels
		if (((chSize && rawEvent->data.DataChannel[ch])||(chSize && rawEvent->fData.DataChannel[ch]))&& !(analysisConfig->useTimeOfFlight && ch==analysisConfig->stopPulseChannel))
		{
			
			//change of sample num (should only happen with first event)
			if (chSize/analysisConfig->samplingReductionFactor!=numSamples)
			{
				//sometimes ch1 has two extra entries, which are zero. Account for this special case by ignoring changes by +2
				if (chSize==numSamples+2)
					break;
				numSamples=chSize/analysisConfig->samplingReductionFactor;
				SAFE_DELETE_ARRAY(tempValArray);
				SAFE_DELETE_ARRAY(tempFilteredValArray);
				tempValArray=new float[numSamples];
				tempFilteredValArray=new float[numSamples];
				memset(tempValArray, 0, sizeof(float)*numSamples);
				memset(tempFilteredValArray, 0, sizeof(float)*numSamples);
			}
			stats->channelStatistics[ch].channelNumber=ch;
			if (analysisConfig->bitsDropped)
			{
				int bitdropFactor=1<<analysisConfig->bitsDropped;

				if (vx1742Mode)
				{
					for (int i=0;i<numSamples;i++)
					{	
						int index=(i)*analysisConfig->samplingReductionFactor;
						tempValArray[i]=0.25f*bitdropFactor*((int)rawEvent->fData.DataChannel[ch][index]/bitdropFactor);
					}
				}
				else
				{
					for (int i=0;i<numSamples;i++)
					{	
						int index=(i)*analysisConfig->samplingReductionFactor;
						tempValArray[i]=bitdropFactor*((int)rawEvent->data.DataChannel[ch][index]/bitdropFactor);
					}
				}
			}
			else
			{
				if (vx1742Mode)
				{
					for (int i=0;i<numSamples;i++)
					{
						int index=(i)*analysisConfig->samplingReductionFactor;
						tempValArray[i]=0.25f*rawEvent->fData.DataChannel[ch][index];
					}
				}
				else				
				{
					for (int i=0;i<numSamples;i++)
					{
						int index=(i)*analysisConfig->samplingReductionFactor;
						tempValArray[i]=((float)rawEvent->data.DataChannel[ch][index]);
					}
				}
			}			
			processSuccess&=findBaseline(tempValArray, 0, analysisConfig->baselineSampleRange, analysisConfig->baselineSampleSize, stats->channelStatistics[ch].baseline);
			
			if (analysisConfig->preCFDFilter)
				processSuccess&=lowPassFilter(tempValArray, numSamples,analysisConfig->preCFDFactor);
			//if the digital gain is not unity
			if (abs(analysisConfig->digitalGain-1.00f)>0.01f)
			{
				processSuccess&=applyGain(tempValArray, numSamples, analysisConfig->digitalGain, stats->channelStatistics[ch].baseline);
                if (analysisConfig->digitalGain<0)
                    stats->channelStatistics[ch].baseline=1024-stats->channelStatistics[ch].baseline;
			}
			//temp hack
			int thresholdCrossing;
			float threshold=990.0f;
			processSuccess&=findIntersection(tempValArray, numSamples, 0, numSamples, 4, threshold, true, thresholdCrossing);


			processSuccess&=clone(tempValArray, numSamples, tempFilteredValArray);
			float CFDThreshold=512.0f;
			processSuccess&=cfdSampleOptimized(tempValArray, stats->channelStatistics[ch].baseline, numSamples, analysisConfig->CFDFraction, analysisConfig->CFDLength, analysisConfig->CFDOffset, CFDThreshold, 1.f/GSPS, tempFilteredValArray);
			if (analysisConfig->postCFDFilter)
				processSuccess&=lowPassFilter(tempFilteredValArray, numSamples,analysisConfig->postCFDFactor);

            int positionOfThresholdCrossing;
			int positionOfCFDMin, positionOfCFDMax;
            float minCFDValue, maxCFDValue;

			processSuccess&=findMinMaxValue(tempFilteredValArray, numSamples, 0, numSamples, minCFDValue, positionOfCFDMin, maxCFDValue, positionOfCFDMax);

            processSuccess&=findIntersection(tempFilteredValArray, numSamples, positionOfCFDMin, positionOfCFDMax, 2, CFDThreshold, false, positionOfThresholdCrossing);
            float crossingPositionInterpolated;
			float cfdIndex=0;
            if (positionOfThresholdCrossing>=positionOfCFDMin)
            {
                float slope, offset;
                //linear fit over 4 points of slope
                processSuccess&=linearFit(tempFilteredValArray, numSamples, positionOfThresholdCrossing-2, positionOfThresholdCrossing+1, slope, offset);
				cfdIndex=(CFDThreshold-offset)/slope;
				int indexLow=(int)(cfdIndex);
				float d=cfdIndex-indexLow;
				
				float time=(indexLow/GSPS)*(1-d)+(d)*((indexLow+1)/GSPS);
				if (analysisConfig->useTimeCFD)
					stats->channelStatistics[ch].timeOfCFDCrossing=time;
				else
					stats->channelStatistics[ch].timeOfCFDCrossing=cfdIndex/GSPS;

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
			{
				stats->channelStatistics[ch].timeOfCFDCrossing=-1000;
				
			}

			processSuccess&=findMinMaxValue(tempValArray, numSamples, 0, numSamples, stats->channelStatistics[ch].minValue, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].maxValue, stats->channelStatistics[ch].indexOfMax);
			
			if (stats->channelStatistics[ch].indexOfMin>0 && stats->channelStatistics[ch].indexOfMin<numSamples)
				stats->channelStatistics[ch].timeOfMin=stats->channelStatistics[ch].indexOfMin/GSPS;

			if (stats->channelStatistics[ch].indexOfMax>0 && stats->channelStatistics[ch].indexOfMax<numSamples)
				stats->channelStatistics[ch].timeOfMax=stats->channelStatistics[ch].indexOfMax/GSPS;

			processSuccess&=findHalfRise(tempValArray, numSamples, stats->channelStatistics[ch].indexOfMin, stats->channelStatistics[ch].baseline, stats->channelStatistics[ch].indexOfHalfRise);
			if (stats->channelStatistics[ch].indexOfHalfRise>0 && stats->channelStatistics[ch].indexOfHalfRise<numSamples)
				stats->channelStatistics[ch].timeOfHalfRise=stats->channelStatistics[ch].indexOfHalfRise/GSPS;

			//0.25 ns per sample
			int samplesPerMicrosecond=(int)(1000*GSPS);
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

			//temp hack
			//timeOffset=thresholdCrossing;
		
			//Charge comparison
			processSuccess&=calculateIntegrals(tempValArray, numSamples, stats->channelStatistics[ch].baseline, timeOffset+startOffset, timeOffset+shortGateOffset, timeOffset+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);			
			//processSuccess&=calculateIntegralsCorrected(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdIndex+startOffset, cfdIndex+shortGateOffset, cfdIndex+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
			//processSuccess&=calculateIntegralsTrapezoidal(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdIndex+startOffset, cfdIndex+shortGateOffset, cfdIndex+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
			//processSuccess&=calculateIntegralsSimpson(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdIndex+startOffset, cfdIndex+shortGateOffset, cfdIndex+longGateOffset, stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral);
			stats->channelStatistics[ch].shortGateIntegral*=analysisConfig->samplingReductionFactor;
			stats->channelStatistics[ch].longGateIntegral*=analysisConfig->samplingReductionFactor;
            processSuccess&=calculatePSD(stats->channelStatistics[ch].shortGateIntegral, stats->channelStatistics[ch].longGateIntegral, stats->channelStatistics[ch].PSD);
			
			//Linear filter
			if (analysisConfig->useOptimalFilter)
			{
				float S=0;
				processSuccess&=linearFilter(tempValArray, numSamples, stats->channelStatistics[ch].baseline, cfdIndex+startOffset, analysisConfig->filter.data(), analysisConfig->filter.size(), S);
				if (stats->channelStatistics[ch].longGateIntegral >0)
					stats->channelStatistics[ch].filteredPSD = 100*S/stats->channelStatistics[ch].longGateIntegral;
			}
									
			//Zero crossing
			//gen
			//float ZCstart=0.152f;
			//float ZCstop=0.662f;
			//62mev
			float ZCstart=0.095f;
			float ZCstop=0.608f;

			float ZCint;
			processSuccess&=calculateZeroCrossing(tempValArray, numSamples, stats->channelStatistics[ch].baseline, timeOffset+startOffset, timeOffset+longGateOffset, stats->channelStatistics[ch].longGateIntegral, ZCstart, ZCstop, ZCint);
			stats->channelStatistics[ch].custom5=ZCint;
			if (outputSample)
			{			
				sample->numSamples=numSamples;
				sample->MSPS=GSPS*1000;
				if (!sample->tValues)
				{
					sample->tValues=new float[numSamples];
					for (int i=0;i<numSamples;i++)
						sample->tValues[i]=i/GSPS;
				}
				sample->fValues[ch]=new float[numSamples];
				//processSuccess&=clone(tempFilteredValArray, numSamples, sample->fValues[ch]);
				if (analysisConfig->displayCFDSignal)
					processSuccess&=clone(tempFilteredValArray, numSamples, sample->fValues[ch]);
				else
					processSuccess&=clone(tempValArray, numSamples, sample->fValues[ch]);

				sample->baseline=stats->channelStatistics[ch].baseline;
				sample->indexStart=timeOffset+startOffset;
				sample->indexShortEnd=timeOffset+shortGateOffset;
				sample->indexLongEnd=timeOffset+longGateOffset;
				sample->cfdTime=timeOffset/GSPS;
			}

			if (analysisConfig->bPostChannelV8)
				runV8CodePostChannelAnalysis(&(stats->channelStatistics[ch]));

			//do output if necessary
			if (outputCurrentChannel)
			{
				printTruncatedSamples(tempValArray, stats->channelStatistics[ch].baseline, timeOffset+startOffset, timeOffset+longGateOffset);
				outputCurrentChannel=false;
			}
			else if (outputCurrentChannelUncorrected)
			{
				printTruncatedSamples(tempValArray, 0, 0, numSamples-1);
				outputCurrentChannelUncorrected=false;
			}
		}
		//write -1 to channel number => no data
		else
			stats->channelStatistics[ch].channelNumber=-1;
	}

	//Time of flight
	int chTOF=analysisConfig->stopPulseChannel;
	if (analysisConfig->useTimeOfFlight && rawEvent->data.ChSize[chTOF] && rawEvent->data.ChSize[chTOF]==numSamples)
	{
		for (int i=0;i<numSamples;i++)
		{				
			tempValArray[i]=((float)rawEvent->data.DataChannel[chTOF][i]);
		}
		if (outputSample)
		{
			sample->fValues[chTOF]=new float[numSamples];

			processSuccess&=clone(tempValArray, numSamples, sample->fValues[chTOF]);
		}
		for (int ch=0;ch<NUM_DIGITIZER_CHANNELS;ch++)
		{	
			SampleStatistics& statsCh=stats->channelStatistics[ch];
			//ignore empty and stop pulse channels
			if (!rawEvent->data.ChSize[ch] || ch==chTOF)
				continue;
			
			float startTime;

			switch (analysisConfig->timeOffsetPulse)
			{
			case CFD_TIME:
				startTime=statsCh.timeOfCFDCrossing+analysisConfig->startOffSetPulse;
				break;
			case TIME_HALF_RISE:
				startTime=statsCh.timeOfHalfRise+analysisConfig->startOffSetPulse;
				break;
			default:
				startTime=statsCh.timeOfMin+analysisConfig->startOffSetPulse;
				break;
			}
			
			int startIndex=startTime*GSPS;
			
			if (outputSample)
				sample->indexOfTOFStartPulse=startIndex;

			int positionOfPulseThresholdCrossing;
			processSuccess&=findIntersection(tempValArray, numSamples, startIndex, numSamples-1, 2, analysisConfig->stopPulseThreshold, true, positionOfPulseThresholdCrossing);
            float pulseCrossingPositionInterpolated;
			float indexTOF=-1;
			float stopTime=positionOfPulseThresholdCrossing/GSPS;
			
            if (positionOfPulseThresholdCrossing>=startIndex)
            {
                float slope, offset;
                //linear fit over 4 points of slope
                processSuccess&=linearFit(tempValArray, numSamples, positionOfPulseThresholdCrossing-2, positionOfPulseThresholdCrossing+1, slope, offset);
				indexTOF=(analysisConfig->stopPulseThreshold-offset)/slope;
				int indexLow=(int)(indexTOF);
				float d=indexTOF-indexLow;				
				stopTime=(indexLow/GSPS)*(1-d)+(d)*((indexLow+1)/GSPS);
			}
			//successful TOF calculation
			
			if (stopTime>0)
			{
				statsCh.timeOfFlight=stopTime-startTime;
				if (outputSample)
				{				
					sample->indexOfTOFEndPulse=stopTime*GSPS;
				}				
			}
			else
			{
				statsCh.timeOfFlight=-1;
				if (outputSample)
				{
					sample->indexOfTOFEndPulse=stopTime*GSPS;
				}
			}			
		}
	}

	if (analysisConfig->bPostEventV8)
		runV8CodePostEventAnalysis(stats);

	if (outputSample)
	{
		sample->stats=*stats;
		emit newEventSample(sample);
	}	
	processedEvents->push_back(stats);
	//delete stats;
}

bool VxProcessThread::runV8CodeInitial()
{
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context=Local<Context>::New(isolate, persContext);

	Handle<Script> handleScript=Local<Script>::New(isolate, scriptInitial);
	Context::Scope context_scope(context);

	handleScript->Run();
	//todo: exceptions
	return true;
}
bool VxProcessThread::runV8CodePreAnalysis()
{
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context=Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript=Local<Script>::New(isolate, scriptPre);
	Context::Scope context_scope(context);

	handleScript->Run();

	return true;
}

bool VxProcessThread::runV8CodePostChannelAnalysis(SampleStatistics* chStats)
{
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context=Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript=Local<Script>::New(isolate, scriptPostChannel);
	Context::Scope context_scope(context);

	Local<Object> obj=Local<Object>::New(isolate, sampleStatsObject);
	obj->SetInternalField(0, External::New(isolate, chStats));
	handleScript->Run();	

	return true;
}
bool VxProcessThread::runV8CodePostEventAnalysis(EventStatistics* evStats)
{
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context=Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript=Local<Script>::New(isolate, scriptPostEvent);
	Context::Scope context_scope(context);

	Local<Object> obj=Local<Object>::New(isolate, eventStatsObject);
	obj->SetInternalField(0, External::New(isolate, evStats));
	handleScript->Run();	

	return true;
}

bool VxProcessThread::runV8CodeFinal()
{
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	v8::Local<v8::ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Local<Context> context=Local<Context>::New(isolate, persContext);
	Local<Script> handleScriptFinished=Local<Script>::New(isolate, scriptFinished);	
	Context::Scope context_scope(context);

	SampleStatistics* newSample=new SampleStatistics();
	newSample->custom5=999;
	Local<Object> obj=Local<Object>::New(isolate, sampleStatsObject);
	obj->SetInternalField(0, External::New(isolate, newSample));
	//sampleStatsObject.Reset(isolate, obj);

	handleScriptFinished->Run();
	
	//todo: exceptions
	return true;
}

bool VxProcessThread::runV8CodeDefinitions()
{
	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(isolate, globalTemplate);
	Handle<Context> context=Local<Context>::New(isolate, persContext);
	Handle<Script> handleScript=Local<Script>::New(isolate, scriptDef);
	Context::Scope context_scope(context);

	handleScript->Run();

	return true;
}

void VxProcessThread::printTruncatedSamples(float* sampleArray, float baseline, float startIndex, float endIndex)
{
	int numOutputSamples=(endIndex-startIndex+1);
	if ((!textStream &&!dataStream) || !file || numOutputSamples<1)
		return;
	bool binaryOutput=dataStream;
	if (binaryOutput)
		dataStream->writeRawData((const char*)&numOutputSamples, sizeof(int));
	if (binaryOutput)
	{
		for (int i=startIndex;i<=endIndex;i++)
		{
			float val=baseline-sampleArray[i];
			dataStream->writeRawData((const char*)&val, sizeof(float));			
		}
	}
	else
	{
		for (int i=startIndex;i<=endIndex;i++)
		{
			float val=baseline-sampleArray[i];
				(*textStream)<<val<<" ";
			(*textStream)<<endl;
		}
	}
}