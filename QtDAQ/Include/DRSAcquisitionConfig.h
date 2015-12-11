#pragma once

#include <QDataStream>

#include "DRS4/DRS.h"
#include "AcquisitionDefinitions.h"


class DRSAcquisitionConfig
{
public:
	DRSAcquisitionConfig();
	static DRSAcquisitionConfig* DefaultConfig();
	~DRSAcquisitionConfig();

	//Synchronisation settings
	bool requiresReconfig = false;

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
	bool channelEnabled[NUM_DIGITIZER_CHANNELS_DRS];
	bool channelSelfTriggerEnabled[NUM_DIGITIZER_CHANNELS_DRS];

	//send config to the board
	bool apply(DRSBoard* board);
};
Q_DECLARE_METATYPE(DRSAcquisitionConfig)

QDataStream &operator<<(QDataStream &out, const DRSAcquisitionConfig &obj);
QDataStream &operator>>(QDataStream &in, DRSAcquisitionConfig &obj);