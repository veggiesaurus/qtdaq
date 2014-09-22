#include "globals.h"

bool useCustomParameterName[]={false,false,false,false, false};
QString customParameterName[]={"","","","",""};

QString axisNameFromParameter(HistogramParameter parameter)
{
	QString axisName;
	switch (parameter)
	{	
	case LONG_INTEGRAL:
		axisName="Long Integral (Energy)";
		break;
	case SHORT_INTEGRAL:
		axisName="Short Integral";
		break;	
	case PSD_FACTOR:
		axisName = "Pulse Shape";
		break;
	case FILTERED_PSD_FACTOR:
		axisName+="Filtered PSD";
		break;
	case BASELINE:
		axisName="Baseline";
		break;
	case TIME_OF_CFD:
		axisName="Time of CFD Crossing (ns)";
		break;
	case CFD_DELTA:
		axisName="CFD Delta (Prev Channel)";
		break;
	case MAX_VALUE:
		axisName="Max Value";
		break;
	case MIN_VALUE:
		axisName="Min Value";
		break;
	case TIME_OF_MIN:
		axisName="Time of Min (ns)";
		break;
	case TIME_OF_MAX:
		axisName="Time of Max (ns)";
		break;
	case HALF_RISE_VALUE:
		axisName="Half-Rise Value";
		break;
	case TIME_OF_HALF_RISE:
		axisName="Time of Half-Rise (ns)";
		break;
	case TEMPERATURE:
		axisName="Temperature";
		break;
	case SECONDS_FROM_FIRST_EVENT:
		axisName="Time from First Event (s)";
		break;
	case TIME_OF_FLIGHT:
		axisName="Time of Flight (ns)";
		break;
	case CUSTOM_1:
		if (useCustomParameterName[0])
			axisName=customParameterName[0];
		else
			axisName="Custom Variable 1";
		break;
	case CUSTOM_2:
		if (useCustomParameterName[1])
			axisName=customParameterName[1];
		else
			axisName="Custom Variable 2";
		break;
	case CUSTOM_3:
		if (useCustomParameterName[2])
			axisName=customParameterName[2];
		else
			axisName="Custom Variable 3";
		break;
	case CUSTOM_4:
		if (useCustomParameterName[3])
			axisName=customParameterName[3];
		else
			axisName="Custom Variable 4";
		break;
	case CUSTOM_5:
		if (useCustomParameterName[4])
			axisName=customParameterName[4];
		else
			axisName="Custom Variable 5";
		break;
	default:
		axisName="Unknown variable";
		break;
	}
	return axisName;
}

double valueFromStatsAndParameter(SampleStatistics stats, HistogramParameter parameter)
{
	double val=0;
	switch (parameter)
	{
	case LONG_INTEGRAL:
		val=stats.longGateIntegral;
		break;
	case SHORT_INTEGRAL:
		val=stats.shortGateIntegral;
		break;
	case PSD_FACTOR:
		val=stats.PSD;
		break;
	case FILTERED_PSD_FACTOR:
		val=stats.filteredPSD;
		break;
	case BASELINE:
		val=stats.baseline;
		break;		
	case TIME_OF_CFD:
		val=stats.timeOfCFDCrossing;
		break;
	case CFD_DELTA:
		val=stats.deltaTprevChannelCFD;
		break;
	case MAX_VALUE:
		val=stats.maxValue;
		break;
	case MIN_VALUE:
		val=stats.minValue;
		break;
	case TIME_OF_MIN:
		val=stats.timeOfMin;
		break;
	case TIME_OF_MAX:
		val=stats.timeOfMax;
		break;
	case HALF_RISE_VALUE:
		val=stats.halfRiseValue;
		break;
	case TIME_OF_HALF_RISE:
		val=stats.timeOfHalfRise;
		break;
	case TEMPERATURE:
		val=stats.temperature;
		break;
	case SECONDS_FROM_FIRST_EVENT:
		val=stats.secondsFromFirstEvent;
		break;
	case TIME_OF_FLIGHT:
		val=stats.timeOfFlight;
		break;
	case CUSTOM_1:
		val=stats.custom1;
		break;
	case CUSTOM_2:
		val=stats.custom2;
		break;
	case CUSTOM_3:
		val=stats.custom3;
		break;
	case CUSTOM_4:
		val=stats.custom4;
		break;
	case CUSTOM_5:
		val=stats.custom5;
		break;
	default:
		val=0;
		break;
	}
	return val;
}

QString abbrFromParameter(HistogramParameter parameter)
{
	QString abbr;
	switch (parameter)
	{
	case LONG_INTEGRAL:
		abbr="ql";
		break;
	case SHORT_INTEGRAL:
		abbr="qs";
		break;
	case PSD_FACTOR:
		abbr="psd";
		break;
	case FILTERED_PSD_FACTOR:
		abbr="filtered_psd";
		break;
	case BASELINE:
		abbr="base";
		break;
	case TIME_OF_CFD:
		abbr="t_cfd";
		break;
	case CFD_DELTA:
		abbr="delta_t_cfd";
	case MAX_VALUE:
		abbr="max";
		break;
	case MIN_VALUE:
		abbr="min";
		break;
	case TIME_OF_MIN:
		abbr="t_min";
		break;
	case TIME_OF_MAX:
		abbr="t_max";
		break;
	case HALF_RISE_VALUE:
		abbr="half";
		break;
	case TIME_OF_HALF_RISE:
		abbr="t_half";
		break;
	case TEMPERATURE:
		abbr="temp";
		break;
	case SECONDS_FROM_FIRST_EVENT:
		abbr="time";
		break;
	case TIME_OF_FLIGHT:
		abbr="tof";
		break;
	case CUSTOM_1:
		abbr="custom1";
		break;
	case CUSTOM_2:
		abbr="custom2";
		break;
	case CUSTOM_3:
		abbr="custom3";
		break;
	case CUSTOM_4:
		abbr="custom4";
		break;
	case CUSTOM_5:
		abbr="custom5";
		break;
	}
	return abbr;
}

float getSecondsFromFirstEvent(EventTimestamp firstEvent, EventTimestamp currentEvent)
{
	QDate dateFirst(firstEvent.year, firstEvent.month, firstEvent.day);
	QDate dateCurrent(currentEvent.year, currentEvent.month, currentEvent.day);
	QTime timeFirst(firstEvent.hour, firstEvent.minute, firstEvent.second, firstEvent.millisecond);
	QTime timeCurrent(currentEvent.hour, currentEvent.minute, currentEvent.second, currentEvent.millisecond);
	int millis=(int)(QDateTime(dateCurrent, timeCurrent).toMSecsSinceEpoch()-QDateTime(dateFirst, timeFirst).toMSecsSinceEpoch());
	return millis/1000.0f;
}

