#include "AcquisitionConfig.h"

AcquisitionConfig::AcquisitionConfig()
{
	/* Default settings */
	samplesPerEvent = (1024);
	sampleRateMSPS=1000;
	postTriggerDelay = 0;
	externalTriggeringEnabled = false;
	triggerPolarityIsNegative = false;
	//3: all corrections
    correctionLevel = 0;
	selfTriggeringEnabled=false;
	voltageOffset=0;
	triggerThreshold=0;
	memset (channelEnabled, 0, sizeof(bool)*NUM_DIGITIZER_CHANNELS);
	memset (channelSelfTriggerEnabled, 0, sizeof(bool)*NUM_DIGITIZER_CHANNELS);
}

AcquisitionConfig::~AcquisitionConfig()
{
}

//default single channel config, with a single self-triggered channel (Ch0), bias to accept negative pulses.
AcquisitionConfig* AcquisitionConfig::DefaultConfig()
{
	AcquisitionConfig* defConfig=new AcquisitionConfig();
	defConfig->correctionLevel=3;
	defConfig->selfTriggeringEnabled=true;
	defConfig->channelEnabled[0]=true;
	defConfig->channelEnabled[1]=true;
	defConfig->channelSelfTriggerEnabled[0]=true;
	defConfig->channelSelfTriggerEnabled[1]=true;
	defConfig->sampleRateMSPS=1000;
	defConfig->triggerThreshold=-60;
	defConfig->triggerPolarityIsNegative=true;
	defConfig->voltageOffset=0;
	defConfig->postTriggerDelay=60;
	return defConfig;
}

//send config to the board
bool AcquisitionConfig::SetDRS4Config(DRSBoard* board)
{
	board->SetFrequency(sampleRateMSPS/1000.0, true);
	//board->SetTranspMode(1);
	board->SetInputRange(voltageOffset/1000.0);
	board->EnableTrigger(selfTriggeringEnabled, externalTriggeringEnabled);
	board->SetTriggerLevel(triggerThreshold/1000.0, triggerPolarityIsNegative); 
	board->SetTriggerDelayPercent(postTriggerDelay);
	int triggerSourceFlag=0;
	for (int i=0;i<NUM_DIGITIZER_CHANNELS;i++)
	{		
		if (channelSelfTriggerEnabled[i])
			triggerSourceFlag|=(1<<i);
	}
    board->SetTriggerSource(triggerSourceFlag);

	return true;
	//todo: corrections
}