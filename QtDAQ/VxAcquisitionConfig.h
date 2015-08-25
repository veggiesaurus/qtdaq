#pragma once

#include "AcquisitionDefinitions.h"
#include <CAENDigitizer.h>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QDebug>

#define MAX_CH  64          /* max. number of channels */
#define MAX_SET 16           /* max. number of independent settings */

#define MAX_GW  1000        /* max. number of generic write commads */



struct VxParseError
{
	enum PARSE_ERROR_TYPE
	{
		WARNING = 0,
		NON_CRITICAL = 1,
		CRITICAL = 2
	};
	PARSE_ERROR_TYPE errorType;
	int lineNumber;
	QString errorMessage;
};

struct VxAcquisitionConfig
{
	CAEN_DGTZ_ConnectionType LinkType;
	int LinkNum;
	int ConetNode;
	uint32_t BaseAddress;
	int Nch;
	int Nbit;
	float Ts;
	int NumEvents;
	int RecordLength;
	int PostTrigger;
	int InterruptNumEvents;
	bool TestPattern;
	bool DesMode;
	CAEN_DGTZ_TriggerPolarity_t TriggerEdge;
	CAEN_DGTZ_IOLevel_t FPIOtype;
	CAEN_DGTZ_TriggerMode_t ExtTriggerMode;
	uint16_t EnableMask;
	CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[MAX_SET];
	uint32_t DCoffset[MAX_SET];
	uint32_t Threshold[MAX_SET];

	uint32_t FTDCoffset[MAX_SET];
	uint32_t FTThreshold[MAX_SET];
	CAEN_DGTZ_TriggerMode_t	FastTriggerMode;
	uint32_t	 FastTriggerEnabled;
	int GWn;
	uint32_t GWaddr[MAX_GW];
	uint32_t GWdata[MAX_GW];
	uint32_t GWmask[MAX_GW];
	VxAcquisitionConfig();
	static VxAcquisitionConfig* parseConfigString(QString configString, QVector<VxParseError>& parseErrors);
};