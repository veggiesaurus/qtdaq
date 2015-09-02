#include "AnalysisConfig.h"

AnalysisConfig::AnalysisConfig()
{
	digitalGain=1.0;
	baselineSampleSize=50;
	baselineSampleRange=75;
	preCFDFilter=false;
	postCFDFilter=false;
	preCFDFactor=0.3;
	postCFDFactor=0.3;
	CFDFraction=0.75;
	CFDLength=6;
	CFDOffset=6;
	useTimeCFD=false;
	timeOffset=CFD_TIME;
	startGate=-10;
	shortGate=20;
	longGate=200;
	useTempCorrection=false;
	referenceTemperature=25;
	scalingVariation=3.0;
	useSaturationCorrection=false;
	saturationLimitQL=200000;
	PDE=35.0;
	numPixels=18900;
	useTimeOfFlight=false;
	stopPulseChannel=0;
	timeOffsetPulse=CFD_TIME;
	startOffSetPulse=5;
	stopPulseThreshold=512;
	
	bInitialV8=bPreAnalysisV8=bPostChannelV8=bPostEventV8=bFinalV8=bDefV8=false;
	//advanced
	bitsDropped=0;
	samplingReductionFactor=1;
	useOptimalFilter=false;
	displayCFDSignal=false;
	primaryCFDChannel = -1;
}

QDataStream &operator<<(QDataStream &out, const AnalysisConfig &obj)
{
	out	<<obj.digitalGain<<obj.baselineSampleSize<<obj.baselineSampleRange<<obj.preCFDFilter
		<<obj.postCFDFilter<<obj.preCFDFactor<<obj.postCFDFactor<<obj.CFDFraction<<obj.CFDLength
		<<obj.CFDOffset<<obj.useTimeCFD<<obj.timeOffset<<(int)obj.startGate<<(int)obj.shortGate<<(int)obj.longGate
		<<obj.useTempCorrection<<obj.referenceTemperature<<obj.scalingVariation<<obj.useSaturationCorrection
		<<obj.saturationLimitQL<<obj.PDE<<obj.numPixels<<obj.useTimeOfFlight<<obj.stopPulseChannel
		<<obj.timeOffsetPulse<<obj.startOffSetPulse<<obj.stopPulseThreshold
		<<obj.bInitialV8<<obj.bFinalV8<<obj.bPreAnalysisV8<<obj.bPostChannelV8<<obj.bPostEventV8<<obj.bDefV8
		<<obj.customCodeInitial<<obj.customCodeFinal<<obj.customCodePreAnalysis<<obj.customCodePostChannel<<obj.customCodePostEvent<<obj.customCodeDef;
     return out;
}
 
QDataStream &operator>>(QDataStream &in, AnalysisConfig &obj)
{
	int t0, tS, tL;

    in  >>obj.digitalGain>>obj.baselineSampleSize>>obj.baselineSampleRange>>obj.preCFDFilter
		>>obj.postCFDFilter>>obj.preCFDFactor>>obj.postCFDFactor>>obj.CFDFraction>>obj.CFDLength
		>>obj.CFDOffset>>obj.useTimeCFD>>*((quint32*)&obj.timeOffset)>>t0>>tS>>tL
		>>obj.useTempCorrection>>obj.referenceTemperature>>obj.scalingVariation>>obj.useSaturationCorrection
		>>obj.saturationLimitQL>>obj.PDE>>obj.numPixels>>obj.useTimeOfFlight>>obj.stopPulseChannel
		>>*((quint32*)&obj.timeOffsetPulse)>>obj.startOffSetPulse>>obj.stopPulseThreshold
		>>obj.bInitialV8>>obj.bFinalV8>>obj.bPreAnalysisV8>>obj.bPostChannelV8>>obj.bPostEventV8>>obj.bDefV8
		>>obj.customCodeInitial>>obj.customCodeFinal>>obj.customCodePreAnalysis>>obj.customCodePostChannel>>obj.customCodePostEvent>>obj.customCodeDef;
	obj.startGate=t0;
	obj.shortGate=tS;
	obj.longGate=tL;
    return in;
}