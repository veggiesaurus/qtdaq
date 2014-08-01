#pragma once

#include <QDataStream>
#include <QVector>

enum RelativeTimeOffset
{
	CFD_TIME=0,
	TIME_HALF_RISE=1,
	TIME_MIN=2
};

struct AnalysisConfig
{
	//baseline
	float digitalGain;
	int baselineSampleSize;
	int baselineSampleRange;
	//filters
	bool preCFDFilter;
	bool postCFDFilter;
	float preCFDFactor;
	float postCFDFactor;
	//CFD
	float CFDFraction;
	int CFDLength;
	int CFDOffset;
	bool useTimeCFD;
	//integrals
	RelativeTimeOffset timeOffset;
	float startGate;
	float shortGate;
	float longGate;
	//temperature correction
	bool useTempCorrection;
	float referenceTemperature;
	float scalingVariation;
	//saturation correction
	bool useSaturationCorrection;
	int saturationLimitQL;
	float PDE;
	float numPixels;
	//time of flight 
	bool useTimeOfFlight;
	int stopPulseChannel;
	RelativeTimeOffset timeOffsetPulse;
	int startOffSetPulse;
	int stopPulseThreshold;
	//custom code
	bool bDefV8,bInitialV8,bPreAnalysisV8,bPostChannelV8,bPostEventV8,bFinalV8;
	QString customCodeDef,customCodeInitial,customCodePreAnalysis,customCodePostChannel,customCodePostEvent,customCodeFinal;	
	//advanced (instance) parameters
	int samplingReductionFactor;
	int bitsDropped;
	//filtering
	bool useOptimalFilter;
	QVector<float> filter;
	//sig display
	bool displayCFDSignal;
	AnalysisConfig();
};
Q_DECLARE_METATYPE(AnalysisConfig)

QDataStream &operator<<(QDataStream &out, const AnalysisConfig &obj);
QDataStream &operator>>(QDataStream &in, AnalysisConfig &obj);