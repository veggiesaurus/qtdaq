#include <QVector>
#include "globals.h"
#include "DRSAcquisitionConfig.h"

DRSAcquisitionConfig::DRSAcquisitionConfig()
{
	/* Default settings */
	samplesPerEvent = (1024);
	sampleRateMSPS=1024;
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

DRSAcquisitionConfig::~DRSAcquisitionConfig()
{
}

//default single channel config, with a single self-triggered channel (Ch0), bias to accept negative pulses.
DRSAcquisitionConfig* DRSAcquisitionConfig::DefaultConfig()
{
	DRSAcquisitionConfig* defConfig=new DRSAcquisitionConfig();
	defConfig->correctionLevel=3;
	defConfig->selfTriggeringEnabled=true;
	defConfig->channelEnabled[0]=true;
	defConfig->channelEnabled[1]=true;
	defConfig->channelSelfTriggerEnabled[0]=true;
	defConfig->channelSelfTriggerEnabled[1]=true;
	defConfig->sampleRateMSPS=1024;
	defConfig->triggerThreshold=-60;
	defConfig->triggerPolarityIsNegative=true;
	defConfig->voltageOffset=0;
	defConfig->postTriggerDelay=60;
	return defConfig;
}

//send config to the board
bool DRSAcquisitionConfig::apply(DRSBoard* board)
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

QDataStream &operator<<(QDataStream &out, const DRSAcquisitionConfig &obj)
{
	out << ACQUISITION_SAVE_VERSION;
	out << obj.correctionLevel << obj.samplesPerEvent << obj.sampleRateMSPS << obj.voltageOffset;
	out << obj.externalTriggeringEnabled << obj.selfTriggeringEnabled << obj.triggerPolarityIsNegative << obj.postTriggerDelay << obj.triggerThreshold;
	out << (quint32)NUM_DIGITIZER_CHANNELS;
	for (auto i = 0; i < NUM_DIGITIZER_CHANNELS; i++)
	{
		out << obj.channelEnabled[i] << obj.channelSelfTriggerEnabled[i];
	}

	return out;
}

QDataStream &operator>>(QDataStream &in, DRSAcquisitionConfig &obj)
{
	//read file version
	quint32 acqFileVersion;
	in >> acqFileVersion;

	in >> obj.correctionLevel >> obj.samplesPerEvent >> obj.sampleRateMSPS >> obj.voltageOffset;
	in >> obj.externalTriggeringEnabled >> obj.selfTriggeringEnabled >> obj.triggerPolarityIsNegative >> obj.postTriggerDelay >> obj.triggerThreshold;

	//careful reading just in case NUM_DIGITIZER CHANNELS is different from numChannels
	quint32 numChannels;
	in >> numChannels;
	for (auto i = 0; i < numChannels; i++)
	{
		bool chEnabled, chTriggerEnabled;
		in >> chEnabled >> chTriggerEnabled;
		if (i < NUM_DIGITIZER_CHANNELS)
		{
			obj.channelEnabled[i] = chEnabled;
			obj.channelSelfTriggerEnabled[i] = chEnabled;
		}
	}	
	return in;
}