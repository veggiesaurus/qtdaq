#include <string.h>
#include <stdio.h>
#include "AcquisitionDefinitions.h"

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

void EventVx::loadFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_UINT16_EVENT_t* data)
{
	this->info = info;
	uint32_t prevChSize[MAX_UINT16_CHANNEL_SIZE];
	memcpy(prevChSize, this->data.ChSize, MAX_UINT16_CHANNEL_SIZE*sizeof(uint16_t));
	memcpy(&(this->info), &(info), sizeof(CAEN_DGTZ_EventInfo_t));
	memcpy(this->data.ChSize, data->ChSize, MAX_UINT16_CHANNEL_SIZE*sizeof(uint16_t));


	for (int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++)
	{	
		int numSamples = data->ChSize[i];		
		if (numSamples)
		{
			if (numSamples != prevChSize[i])
			{
				SAFE_DELETE_ARRAY(this->data.DataChannel[i]);
				this->data.DataChannel[i] = new uint16_t[numSamples];
			}
			memcpy(this->data.DataChannel[i], data->DataChannel[i], numSamples*sizeof(uint16_t));
		}
	}
}

void freeVxEvent(EventVx* &ev)
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

void freeVxEvent(EventVx &ev)
{
	for (int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++)
	{
		if (ev.data.ChSize[i])
			SAFE_DELETE_ARRAY(ev.data.DataChannel[i]);
		if (ev.fData.ChSize[i])
			SAFE_DELETE_ARRAY(ev.fData.DataChannel[i]);
	}
	memset(&ev, 0, sizeof(EventVx));
}

DataHeader::DataHeader()
{
	int x = sizeof(DataHeader);
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