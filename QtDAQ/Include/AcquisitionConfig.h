#pragma once

#include "qstring.h"
#include "DRS4/DRS.h"
#include "DRS4Acquisition.h"

class AcquisitionConfig
{
public:
	AcquisitionConfig();
	static AcquisitionConfig* DefaultConfig();
	~AcquisitionConfig();
	bool loadFromFile(QString filename);
	bool writeToFile(QString filename);

	//Global Digitizer Settings
	int correctionLevel;
	unsigned int samplesPerEvent;	
	int sampleRateMSPS;
	float voltageOffset;

	//Global Trigger Settings
	bool externalTriggeringEnabled;
	bool selfTriggeringEnabled;
	bool triggerPolarityIsNegative;
	float postTriggerDelay;
	float triggerThreshold;
	
	//individual settings
	bool channelEnabled[NUM_DIGITIZER_CHANNELS];
	bool channelSelfTriggerEnabled[NUM_DIGITIZER_CHANNELS];

	//send config to the board
	bool SetDRS4Config(DRSBoard* board);
};

