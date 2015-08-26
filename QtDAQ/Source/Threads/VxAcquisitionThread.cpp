#include "Threads/VxAcquisitionThread.h"

VxAcquisitionThread::VxAcquisitionThread(QMutex* s_rawBuffer1Mutex, QMutex* s_rawBuffer2Mutex, EventVx* s_rawBuffer1, EventVx* s_rawBuffer2, QObject *parent)
	: QThread(parent)
{

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
	return ERR_NONE;
}

void VxAcquisitionThread::run()
{
	CAEN_DGTZ_SWStartAcquisition(handle);
}

void VxAcquisitionThread::onUpdateTimerTimeout()
{

}

void VxAcquisitionThread::setPaused(bool paused)
{
	pauseMutex.lock();
	requiresPause = paused;
	pauseMutex.unlock();
}

void VxAcquisitionThread::stopAcquisition()
{

}

bool VxAcquisitionThread::setFileOutput(QString filename, bool useCompression)
{
	return true;
}

CAENErrorCode VxAcquisitionThread::InitDigitizer()
{
	bool isVMEDevice = config->BaseAddress;
	CAEN_DGTZ_ErrorCode retCode;
	retCode = CAEN_DGTZ_OpenDigitizer(config->LinkType, config->LinkNum, config->ConetNode, config->BaseAddress, &handle);
	if (retCode)
		return ERR_DGZ_OPEN;

	retCode = CAEN_DGTZ_GetInfo(handle, boardInfo);
	if (retCode)
		return ERR_BOARD_INFO_READ;

	retCode = GetMoreBoardInfo();
	if (retCode)
		return ERR_INVALID_BOARD_TYPE;
	return ERR_NONE;
}

CAENErrorCode VxAcquisitionThread::ProgramDigitizer()
{
	CAEN_DGTZ_ErrorCode retCode = CAEN_DGTZ_Success;
	retCode |= CAEN_DGTZ_Reset(handle);
	if (retCode)
		return CAENErrorCode::ERR_RESTART;

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
			return ERR_INTERRUPT;
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
		return CAENErrorCode::ERR_DGZ_PROGRAM;
	}
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

void VxAcquisitionThread::CloseDigitizer()
{
	CAEN_DGTZ_SWStopAcquisition(handle);
	if (event16)
		CAEN_DGTZ_FreeEvent(handle, (void**)&event16);
	CAEN_DGTZ_FreeReadoutBuffer(&buffer);
	CAEN_DGTZ_CloseDigitizer(handle);
}
