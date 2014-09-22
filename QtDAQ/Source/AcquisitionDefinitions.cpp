#include "AcquisitionDefinitions.h"
#include "string.h"
#include "stdio.h"

DataHeader::DataHeader()
{
	memset(this, 0, sizeof(DataHeader));
	time(&dateTime);
	sprintf(description, "Test output.");
	sprintf(futureUse, "RSV");
	sprintf(magicNumber, "UCT001B");
	MSPS = 4000;
	numEvents = 2;
	numSamples = 1024;
	offsetDC = 1000;
	peakSaturation = 1;
}

EventVx* EventVx::eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_UINT16_EVENT_t* data)
{
	EventVx* ev = new EventVx();
	memcpy(&(ev->info), &(info), sizeof(CAEN_DGTZ_EventInfo_t));
	memcpy(ev->data.ChSize, data->ChSize, MAX_UINT16_CHANNEL_SIZE*sizeof(uint16_t));

	for (int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++)
	{
		int numSamples = data->ChSize[i];
		if (numSamples)
		{
			ev->data.DataChannel[i] = new uint16_t[numSamples];
			memcpy(ev->data.DataChannel[i], data->DataChannel[i], numSamples*sizeof(uint16_t));
		}
	}
	return ev;
}

EventVx* EventVx::eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_FLOAT_EVENT_t* fData)
{
	EventVx* ev = new EventVx();
	memcpy(&(ev->info), &(info), sizeof(CAEN_DGTZ_EventInfo_t));
	memcpy(ev->fData.ChSize, fData->ChSize, MAX_UINT16_CHANNEL_SIZE*sizeof(uint16_t));

	for (int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++)
	{
		int numSamples = fData->ChSize[i];
		if (numSamples)
		{
			ev->fData.DataChannel[i] = new float[numSamples];
			memcpy(ev->fData.DataChannel[i], fData->DataChannel[i], numSamples*sizeof(float));
		}
	}
	return ev;
}

void freeEvent(EventVx* &ev)
{
	for (int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++)
	{
		if (ev->data.ChSize[i])
			SAFE_DELETE_ARRAY(ev->data.DataChannel[i]);
		if (ev->fData.ChSize[i])
			SAFE_DELETE_ARRAY(ev->fData.DataChannel[i]);
	}
	SAFE_DELETE(ev);
}

void freeEvent(EventVx &ev)
{
	for (int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++)
	{
		if (ev.data.ChSize[i])
			SAFE_DELETE_ARRAY(ev.data.DataChannel[i]);
		if (ev.fData.ChSize[i])
			SAFE_DELETE_ARRAY(ev.fData.DataChannel[i]);
	}
}