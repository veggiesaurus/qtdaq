#pragma once

#include <QDataStream>
#include <QVector>
#include "DRS4/DRS.h"
#include "DRS4Acquisition.h"
#include "globals.h"


class AcquisitionConfig
{
public:
	AcquisitionConfig();
	static AcquisitionConfig* DefaultConfig();
	~AcquisitionConfig();

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
	bool channelEnabled[NUM_DIGITIZER_CHANNELS];
	bool channelSelfTriggerEnabled[NUM_DIGITIZER_CHANNELS];

	//send config to the board
	bool apply(DRSBoard* board);
};
Q_DECLARE_METATYPE(AcquisitionConfig)

QDataStream &operator<<(QDataStream &out, const AcquisitionConfig &obj);
QDataStream &operator>>(QDataStream &in, AcquisitionConfig &obj);