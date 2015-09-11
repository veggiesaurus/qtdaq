#include <QDebug>
#include "globals.h"
#include "Threads/VxAcquisitionThread.h"

VxAcquisitionThread::VxAcquisitionThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, int s_bufferLength, QObject *parent)
	: QThread(parent)
{
	requiresPause = false;
    outputFileCompressed = nullptr;

	rawBuffers[0] = s_rawBuffer1;
	rawBuffers[1] = s_rawBuffer2;
	rawMutexes[0] = s_rawBuffer1Mutex;
	rawMutexes[1] = s_rawBuffer2Mutex;
	bufferLength = s_bufferLength;
}

VxAcquisitionThread::~VxAcquisitionThread()
{

}

CAENErrorCode VxAcquisitionThread::initVxAcquisitionThread(VxAcquisitionConfig* s_config, int s_runIndex, int updateTime)
{	
	if (digitizerStatus&STATUS_INITIALISED)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
	}

	SAFE_DELETE(config);
	SAFE_DELETE(boardInfo);
	SAFE_DELETE(event16);
	SAFE_DELETE(buffer);
	if (!s_config)
		return CAENErrorCode::ERR_CONF_FILE_INVALID;
	config = s_config;
	boardInfo = new CAEN_DGTZ_BoardInfo_t();

	//init the digitizer
	CAENErrorCode errInitDigitizer = InitDigitizer();
	if (errInitDigitizer)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return errInitDigitizer;
	}
	digitizerStatus |= STATUS_INITIALISED;

	CAENErrorCode errProgramDigitizer = ProgramDigitizer();
	if (errProgramDigitizer)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return errInitDigitizer;
	}
	digitizerStatus |= STATUS_PROGRAMMED;
	runIndex = s_runIndex;

	auto retAllocate = CAEN_DGTZ_AllocateEvent(handle, (void**)&event16);
	if (retAllocate)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return ERR_MALLOC;
	}
	retAllocate = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &allocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
	if (retAllocate) {
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return ERR_MALLOC;
	}
	digitizerStatus |= STATUS_READY;

	numEventsAcquired = 0;

	//start reading into buffer0, but need to lock and unlock buffer1 as well to make sure it's free
	rawMutexes[0]->lock();
	rawMutexes[1]->lock();
	for (int i = 0; i < bufferLength; i++)
	{
		rawBuffers[0][i].processed = false;
		rawBuffers[1][i].processed = false;
	}
	rawMutexes[1]->unlock();
	currentBufferIndex = 0;
	currentBufferPosition = 0;
	timeSinceLastBufferSwap.start();

	return ERR_NONE;
}

void VxAcquisitionThread::run()
{	
	while (digitizerStatus & STATUS_READY)
	{
		//pausing
		pauseMutex.lock();
		if (requiresPause)
		{
			pauseMutex.unlock();
			if (digitizerStatus & STATUS_RUNNING)
			{
				digitizerMutex.lock();
				auto retStop = CAEN_DGTZ_SWStopAcquisition(handle);

                if (outputFileCompressed)
                {
                    gzflush(outputFileCompressed, Z_FINISH);
                    gzclose(outputFileCompressed);
                    outputFileCompressed=nullptr;
                    qDebug()<<"Closing file "+filename;
                }


				digitizerStatus &= ~STATUS_RUNNING;
				if (retStop)
				{
                    qDebug()<<"Error stopping acquisition";
					CloseDigitizer();
					digitizerStatus = STATUS_ERROR;
					return;
				}
				digitizerMutex.unlock();
			}
			msleep(100);
			continue;
		}
		else		
			pauseMutex.unlock();


	
		digitizerMutex.lock();
		if (!(digitizerStatus & STATUS_RUNNING) && digitizerStatus & STATUS_READY)
		{
            auto retCalib = CAEN_DGTZ_Calibrate(handle);
            if (retCalib)
            {
                qDebug()<<"Error calibrating digitizer";
                CloseDigitizer();
                digitizerStatus = STATUS_ERROR;
                return;
            }
			auto retStart = CAEN_DGTZ_SWStartAcquisition(handle);
			if (retStart)
			{
                qDebug()<<"Error starting acquisition";
				CloseDigitizer();
				digitizerStatus = STATUS_ERROR;
				return;
			}
			timeSinceLastBufferSwap.restart();
			digitizerStatus |= STATUS_RUNNING;
		}
		//if (timeSinceLastBufferSwap.elapsed() > BUFFER_SWAP_TIME)
			//swapBuffers();

		/* Read data from the board */
		auto ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &bufferSize);
        if (ret)
        {
            qDebug()<<"Error reading data";
			CloseDigitizer();
			digitizerStatus = STATUS_ERROR;
			digitizerMutex.unlock();
			return;
		}
		numEvents = 0;
		if (bufferSize != 0) 
		{
			ret = CAEN_DGTZ_GetNumEvents(handle, buffer, bufferSize, &numEvents);
            if (ret)
            {
                qDebug()<<"Error reading num events";
				CloseDigitizer();
				digitizerStatus = STATUS_ERROR;
				digitizerMutex.unlock();
				return;
			}
		}
		else 
		{
			uint32_t lstatus;
			ret = CAEN_DGTZ_ReadRegister(handle, CAEN_DGTZ_ACQ_STATUS_ADD, &lstatus);
            if (lstatus & (0x1 << 19))
            {
                qDebug()<<"Register read error";
				CloseDigitizer();
				digitizerStatus = STATUS_ERROR;
				digitizerMutex.unlock();
				return;
			}
		}

		for (int i = 0; i < numEvents; i++) 
		{

			/* Get one event from the readout buffer */
			ret = CAEN_DGTZ_GetEventInfo(handle, buffer, bufferSize, i, &eventInfo, &eventPtr);
			if (ret) 
			{
                qDebug()<<"Event info error";
                CloseDigitizer();
				digitizerStatus = STATUS_ERROR;
				digitizerMutex.unlock();
				return;
			}
			/* decode the event */
			ret = CAEN_DGTZ_DecodeEvent(handle, eventPtr, (void**)&event16);
			if (ret)
			{
                qDebug()<<"Event decode error";
				ResetDigitizer();
				break;
			}

			//freeVxEvent(rawBuffers[currentBufferIndex][currentBufferPosition]);
			rawBuffers[currentBufferIndex][currentBufferPosition].loadFromInfoAndData(eventInfo, event16);
			rawBuffers[currentBufferIndex][currentBufferPosition].processed = false;
			rawBuffers[currentBufferIndex][currentBufferPosition].runIndex = runIndex;

            if (outputFileCompressed)
            {
                bool appendSuccess;
                if (packedOutput)
                    appendSuccess = AppendEventPacked(&rawBuffers[currentBufferIndex][currentBufferPosition]);
                else
                    appendSuccess = AppendEventCompressed(&rawBuffers[currentBufferIndex][currentBufferPosition]);
                if (!appendSuccess)
                {
                    CloseDigitizer();
                    qDebug()<<"Error writing to file";
                    digitizerStatus = STATUS_ERROR;
                    digitizerMutex.unlock();
                    return;
                }
            }

			currentBufferPosition++;




            //release and swap buffers when position overflows
            if (currentBufferPosition >= bufferLength)
                swapBuffers();
		}
		digitizerMutex.unlock();
	}

}

void VxAcquisitionThread::swapBuffers()
{
	rawMutexes[currentBufferIndex]->unlock();
	//swap
	currentBufferIndex = 1 - currentBufferIndex;
	rawMutexes[currentBufferIndex]->lock();
	currentBufferPosition = 0;
	timeSinceLastBufferSwap.restart();
}

CAENErrorCode VxAcquisitionThread::ResetDigitizer()
{
	if (digitizerStatus&STATUS_INITIALISED)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
	}
	digitizerMutex.unlock();
	SAFE_DELETE(boardInfo);
	SAFE_DELETE(event16);
	SAFE_DELETE(buffer);
	boardInfo = new CAEN_DGTZ_BoardInfo_t();

	//init the digitizer
	CAENErrorCode errInitDigitizer = InitDigitizer();
	if (errInitDigitizer)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return errInitDigitizer;
	}
	digitizerStatus |= STATUS_INITIALISED;

	CAENErrorCode errProgramDigitizer = ProgramDigitizer();
	if (errProgramDigitizer)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return errInitDigitizer;
	}
	digitizerStatus |= STATUS_PROGRAMMED;

	auto retAllocate = CAEN_DGTZ_AllocateEvent(handle, (void**)&event16);
	if (retAllocate)
	{
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return ERR_MALLOC;
	}
	retAllocate = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &allocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
	if (retAllocate) {
		CloseDigitizer();
		digitizerStatus = STATUS_CLOSED;
		return ERR_MALLOC;
	}
	digitizerStatus |= STATUS_READY;
	digitizerMutex.lock();
	return ERR_NONE;
}

void VxAcquisitionThread::setPaused(bool paused)
{
	pauseMutex.lock();
	requiresPause = paused;
	pauseMutex.unlock();
}

void VxAcquisitionThread::stopAcquisition(bool forcedExit)
{
	digitizerMutex.lock();	
	CloseDigitizer(forcedExit);
	digitizerMutex.unlock();
}

bool VxAcquisitionThread::setFileOutput(QString s_filename, bool s_packedOutput)
{    
    if (outputFileCompressed)
    {
        gzflush(outputFileCompressed, Z_FINISH);
        gzclose(outputFileCompressed);
        outputFileCompressed=nullptr;
        qDebug()<<"Closing file "+filename;
    }

    filename = s_filename;
    packedOutput = s_packedOutput;
    outputFileCompressed=gzopen(filename.toStdString().c_str(), "wb1f");
    if (!outputFileCompressed)
        return false;
    if (packedOutput)
    {
        if (!WriteDataHeaderCompressed())
            return false;
        qDebug()<<"Header written to file "+filename+" successfully";
    }
    else
    {
        if (!WriteDataHeaderPacked())
            return false;
        qDebug()<<"Packed header written to file "+filename+" successfully";
    }
	return true;
}


bool VxAcquisitionThread::WriteDataHeaderCompressed()
{
    if (!gzwrite(outputFileCompressed, &header, sizeof(DataHeader)))
        return false;
    return true;
}

bool VxAcquisitionThread::AppendEventCompressed(EventVx* ev)
{
    if (!gzwrite(outputFileCompressed, &ev->info, sizeof(CAEN_DGTZ_EventInfo_t)))
        return false;
    if (!gzwrite(outputFileCompressed, ev->data.ChSize, sizeof(uint32_t)*MAX_UINT16_CHANNEL_SIZE))
        return false;
    for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
    {
        if (ev->data.ChSize[i] && !gzwrite(outputFileCompressed, ev->data.DataChannel[i], sizeof(uint16_t) * ev->data.ChSize[i]))
            return false;
    }
    return true;
}


bool VxAcquisitionThread::WriteDataHeaderPacked()
{
    sprintf(header.magicNumber, "UCT001P");
    if (!gzwrite(outputFileCompressed, &header, sizeof(DataHeader)))
        return false;
    return true;
}

bool VxAcquisitionThread::AppendEventPacked(EventVx* ev)
{
    if (!gzwrite(outputFileCompressed, &ev->info, sizeof(CAEN_DGTZ_EventInfo_t)))
        return false;
    if (!gzwrite(outputFileCompressed, ev->data.ChSize, sizeof(uint32_t)*MAX_UINT16_CHANNEL_SIZE))
        return false;
    for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
    {
        if (!ev->data.ChSize[i])
            continue;
        if (ev->data.ChSize[i]-1 != packedDataLength)
        {
            SAFE_DELETE_ARRAY(packedData);
            packedDataLength = ev->data.ChSize[i]-1;
            packedData = new int8_t[packedDataLength];
        }
        packChannel(ev->data.DataChannel[i], ev->data.ChSize[i], packedData);
        if (gzwrite(outputFileCompressed, &(ev->data.DataChannel[i][0]), sizeof(uint16_t)) && !gzwrite(outputFileCompressed, packedData, sizeof(u_int8_t) * packedDataLength))
            return false;
    }
    return true;
}

bool VxAcquisitionThread::packChannel(u_int16_t* chData, u_int16_t channelSize, int8_t* destData)
{
    for (auto i = 1; i<channelSize; i++)
    {
        if (chData[i]-chData[i-1]>MAXINT8)
        {
            qDebug()<<"Error packing channel data";
            return false;
        }
        destData[i-1] = (int8_t)(chData[i]-chData[i-1]);
    }
    return true;
}



CAENErrorCode VxAcquisitionThread::InitDigitizer()
{
	bool isVMEDevice = config->BaseAddress;
	CAEN_DGTZ_ErrorCode retCode;
	digitizerMutex.lock();
	retCode = CAEN_DGTZ_OpenDigitizer(config->LinkType, config->LinkNum, config->ConetNode, config->BaseAddress, &handle);
	if (retCode)
	{
		digitizerMutex.unlock();
		return ERR_DGZ_OPEN;
	}
	retCode = CAEN_DGTZ_GetInfo(handle, boardInfo);
	if (retCode)
	{
		digitizerMutex.unlock();
		return ERR_BOARD_INFO_READ;
	}
	retCode = GetMoreBoardInfo();
	if (retCode)
	{
		digitizerMutex.unlock();
		return ERR_INVALID_BOARD_TYPE;
	}
	digitizerMutex.unlock();
	return ERR_NONE;
}

CAENErrorCode VxAcquisitionThread::ProgramDigitizer()
{
	CAEN_DGTZ_ErrorCode retCode = CAEN_DGTZ_Success;
	digitizerMutex.lock();
	retCode |= CAEN_DGTZ_Reset(handle);
	if (retCode)
	{
		digitizerMutex.unlock();
		return ERR_RESTART;
	}
	// Set the waveform test bit for debugging
	if (config->TestPattern)
		retCode |= CAEN_DGTZ_WriteRegister(handle, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1 << 3);	
	retCode |= CAEN_DGTZ_SetRecordLength(handle, config->RecordLength);
	retCode |= CAEN_DGTZ_GetRecordLength(handle, &config->RecordLength);
	

	retCode |= CAEN_DGTZ_SetPostTriggerSize(handle, config->PostTrigger);
	retCode |= CAEN_DGTZ_SetIOLevel(handle, config->FPIOtype);
	if (config->InterruptNumEvents > 0) 
	{
		// Interrupt handling
		if (CAEN_DGTZ_SetInterruptConfig(handle, CAEN_DGTZ_ENABLE, VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID, config->InterruptNumEvents, INTERRUPT_MODE) != CAEN_DGTZ_Success) 
		{
			digitizerMutex.unlock();
			return ERR_INTERRUPT;
		}
	}
	retCode |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, config->NumEvents);
	retCode |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
	retCode |= CAEN_DGTZ_SetExtTriggerInputMode(handle, config->ExtTriggerMode);

	
	retCode |= CAEN_DGTZ_SetChannelEnableMask(handle, config->EnableMask);
	for (int i = 0; i < config->Nch; i++) 
	{
		if (config->EnableMask & (1 << i)) 
		{
			retCode |= CAEN_DGTZ_SetChannelDCOffset(handle, i, config->DCoffset[i]);
			if (boardInfo->FamilyCode != CAEN_DGTZ_XX730_FAMILY_CODE)
				retCode |= CAEN_DGTZ_SetChannelSelfTrigger(handle, config->ChannelTriggerMode[i], (1 << i));
			retCode |= CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, config->Threshold[i]);
			retCode |= CAEN_DGTZ_SetTriggerPolarity(handle, i, config->TriggerEdge);
		}
	}
	if (boardInfo->FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE) 
	{
		// channel pair settings for x730 boards
		for (int i = 0; i < config->Nch; i += 2) {
			if (config->EnableMask & (0x3 << i)) {
				CAEN_DGTZ_TriggerMode_t mode = config->ChannelTriggerMode[i];
				uint32_t pair_chmask = 0;

				// Build mode and relevant channelmask. The behaviour is that,
				// if the triggermode of one channel of the pair is DISABLED,
				// this channel doesn't take part to the trigger generation.
				// Otherwise, if both are different from DISABLED, the one of
				// the even channel is used.
				if (config->ChannelTriggerMode[i] != CAEN_DGTZ_TRGMODE_DISABLED) {
					if (config->ChannelTriggerMode[i + 1] == CAEN_DGTZ_TRGMODE_DISABLED)
						pair_chmask = (0x1 << i);
					else
						pair_chmask = (0x3 << i);
				}
				else {
					mode = config->ChannelTriggerMode[i + 1];
					pair_chmask = (0x2 << i);
				}

				pair_chmask &= config->EnableMask;
				retCode |= CAEN_DGTZ_SetChannelSelfTrigger(handle, mode, pair_chmask);
			}
		}
	}
	
	/* execute generic write commands */
	for (int i = 0; i < config->GWn; i++)
		retCode |= WriteRegisterBitmask(config->GWaddr[i], config->GWdata[i], config->GWmask[i]);

	if (retCode)
	{
		qDebug() << "Warning: errors found during the programming of the digitizer.\nSome settings may not be executed";	
		digitizerMutex.unlock();
		return ERR_DGZ_PROGRAM;
	}
	digitizerMutex.unlock();
	return ERR_NONE;
}

CAEN_DGTZ_ErrorCode VxAcquisitionThread::WriteRegisterBitmask(uint32_t address, uint32_t data, uint32_t mask) {
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	uint32_t d32 = 0xFFFFFFFF;

	ret = CAEN_DGTZ_ReadRegister(handle, address, &d32);
	if (ret != CAEN_DGTZ_Success)
		return ret;

	data &= mask;
	d32 &= ~mask;
	d32 |= data;
	ret = CAEN_DGTZ_WriteRegister(handle, address, d32);
	return ret;
}

CAEN_DGTZ_ErrorCode VxAcquisitionThread::GetMoreBoardInfo()
{
	int ret;
	switch (boardInfo->FamilyCode) {

	case CAEN_DGTZ_XX761_FAMILY_CODE: config->Nbit = 10; config->Ts = 0.25;  break;
	case CAEN_DGTZ_XX730_FAMILY_CODE: config->Nbit = 14; config->Ts = 2.0; break;	
	default: return CAEN_DGTZ_BadBoardType;
	}
	
	switch (boardInfo->FamilyCode) {
	case CAEN_DGTZ_XX761_FAMILY_CODE:
		switch (boardInfo->FormFactor) {
		case CAEN_DGTZ_VME64_FORM_FACTOR:
		case CAEN_DGTZ_VME64X_FORM_FACTOR:
			config->Nch = 8;
			break;
		case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
		case CAEN_DGTZ_NIM_FORM_FACTOR:
			config->Nch = 4;
			break;
		}
		break;
	case CAEN_DGTZ_XX730_FAMILY_CODE:
		switch (boardInfo->FormFactor) {
		case CAEN_DGTZ_VME64_FORM_FACTOR:
		case CAEN_DGTZ_VME64X_FORM_FACTOR:
			config->Nch = 16;
			break;
		case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
		case CAEN_DGTZ_NIM_FORM_FACTOR:
			config->Nch = 8;
			break;
		}
		break;	
	default:
		return CAEN_DGTZ_BadBoardType;
	}
	return CAEN_DGTZ_Success;
}

void VxAcquisitionThread::CloseDigitizer(bool finalClose)
{
	digitizerStatus = STATUS_CLOSED;
	if (handle >= 0)
	{	
		CAEN_DGTZ_SWStopAcquisition(handle);
		if (event16)
			CAEN_DGTZ_FreeEvent(handle, (void**)&event16);
	}
	if (!finalClose && buffer)
		CAEN_DGTZ_FreeReadoutBuffer(&buffer);
	if (handle >= 0)
		CAEN_DGTZ_CloseDigitizer(handle);
	SAFE_DELETE(boardInfo);
}
