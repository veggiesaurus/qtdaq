/*
* Most of this comes straight from the CAEN WaveDump example
*/


#include "CAEN/CAENAquisition.h"



#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

/* ###########################################################################
*  Functions
*  ########################################################################### */
/*! \fn      static long get_time()
*   \brief   Get time in milliseconds
*
*   \return  time in msec
*/
static long get_time()
{
    long time_ms;
#ifdef WIN32
    struct _timeb timebuffer;
    _ftime( &timebuffer );
    time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
#else
    struct timeval t1;
    struct timezone tz;
    gettimeofday(&t1, &tz);
    time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
#endif
    return time_ms;
}

/*! \fn      CAEN_DGTZ_ErrorCode GetMoreBoardNumChannels(CAEN_DGTZ_BoardInfo_t boardInfo,  CAENConfig *caenCfg)
*   \brief   calculate num of channels, num of bit and sampl period according to the board type
*
*   \param   boardInfo   Board Type
*   \param   caenCfg       pointer to the config. struct
*   \return  0 = Success; CAEN_DGTZ_BadBoardType = unknown board type
*/
CAEN_DGTZ_ErrorCode GetMoreboardInfo(int* handle, CAEN_DGTZ_BoardInfo_t* boardInfo, CAENConfig *caenCfg)
{
	CAEN_DGTZ_DRS4Frequency_t freq;
	int ret;
    switch(boardInfo->FamilyCode) {
        case CAEN_DGTZ_XX724_FAMILY_CODE: caenCfg->NumBits = 14; caenCfg->Ts = 10.0; break;
        case CAEN_DGTZ_XX720_FAMILY_CODE: caenCfg->NumBits = 12; caenCfg->Ts = 4.0;  break;
        case CAEN_DGTZ_XX721_FAMILY_CODE: caenCfg->NumBits =  8; caenCfg->Ts = 2.0;  break;
        case CAEN_DGTZ_XX731_FAMILY_CODE: caenCfg->NumBits =  8; caenCfg->Ts = 2.0;  break;
        case CAEN_DGTZ_XX751_FAMILY_CODE: caenCfg->NumBits = 10; caenCfg->Ts = 1.0;  break;
        case CAEN_DGTZ_XX761_FAMILY_CODE: caenCfg->NumBits = 10; caenCfg->Ts = 0.25;  break;
        case CAEN_DGTZ_XX740_FAMILY_CODE: caenCfg->NumBits = 12; caenCfg->Ts = 16.0; break;
        case CAEN_DGTZ_XX742_FAMILY_CODE: 
        caenCfg->NumBits = 12; 
        if ((ret = CAEN_DGTZ_GetDRS4SamplingFrequency(*handle, &freq)) != CAEN_DGTZ_Success) return CAEN_DGTZ_CommError;
		switch (freq) {
				case CAEN_DGTZ_DRS4_1GHz:
						caenCfg->Ts = 1.0;
					break;
				case CAEN_DGTZ_DRS4_2_5GHz:
						caenCfg->Ts = (float)0.4;
					break;
				case CAEN_DGTZ_DRS4_5GHz:
						caenCfg->Ts = (float)0.2;
					break;
		}
        break;
        default: return CAEN_DGTZ_BadBoardType;
    }
    if (((boardInfo->FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) ||
         (boardInfo->FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) ) && caenCfg->DesMode)
        caenCfg->Ts /= 2;

    switch(boardInfo->FamilyCode) {
        case CAEN_DGTZ_XX724_FAMILY_CODE:
        case CAEN_DGTZ_XX720_FAMILY_CODE:
        case CAEN_DGTZ_XX721_FAMILY_CODE:
        case CAEN_DGTZ_XX751_FAMILY_CODE:
        case CAEN_DGTZ_XX761_FAMILY_CODE:
        case CAEN_DGTZ_XX731_FAMILY_CODE:
            switch(boardInfo->FormFactor) {
                case CAEN_DGTZ_VME64_FORM_FACTOR:
                case CAEN_DGTZ_VME64X_FORM_FACTOR:
                    caenCfg->NumChannels = 8;
                    break;
                case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
                case CAEN_DGTZ_NIM_FORM_FACTOR:
                    caenCfg->NumChannels = 4;
                    break;
                }
            break;
        case CAEN_DGTZ_XX740_FAMILY_CODE:
            switch( boardInfo->FormFactor) {
            case CAEN_DGTZ_VME64_FORM_FACTOR:
            case CAEN_DGTZ_VME64X_FORM_FACTOR:
                caenCfg->NumChannels = 64;
                break;
            case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
            case CAEN_DGTZ_NIM_FORM_FACTOR:
                caenCfg->NumChannels = 32;
                break;
            }
            break;
        case CAEN_DGTZ_XX742_FAMILY_CODE:
            switch( boardInfo->FormFactor) {
            case CAEN_DGTZ_VME64_FORM_FACTOR:
            case CAEN_DGTZ_VME64X_FORM_FACTOR:
                caenCfg->NumChannels = 36;
                break;
            case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
            case CAEN_DGTZ_NIM_FORM_FACTOR:
                caenCfg->NumChannels = 16;
                break;
            }
            break;
        default:
            return CAEN_DGTZ_BadBoardType;
    }
    return CAEN_DGTZ_Success;
}

/*! \fn      int ProgramDigitizer(int handle, WaveDumpConfig_t caenCfg)
*   \brief   configure the digitizer according to the parameters read from
*            the cofiguration file and saved in the caenCfg data structure
*
*   \param   handle   Digitizer handle
*   \param   caenCfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
CAENErrorCode ProgramDigitizer(int handle, CAENConfig* caenCfg, CAEN_DGTZ_BoardInfo_t* boardInfo, QTextStream& textStream)
{
    int i,j, ret = 0;

    /* reset the digitizer */
    ret |= CAEN_DGTZ_Reset(handle);
	if (ret != 0)
	{
		textStream<<"Error: Unable to reset digitizer"<<endl<<"Please reset digitizer manually then restart the program"<<endl;
		return ERR_RESTART;
	}
    /* execute generic write commands */
    for(i=0; i<caenCfg->GWn; i++)
        ret |= CAEN_DGTZ_WriteRegister(handle, caenCfg->GWaddr[i], caenCfg->GWdata[i]);

    // Set the waveform test bit for debugging
    if (caenCfg->TestPattern)
        ret |= CAEN_DGTZ_WriteRegister(handle, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
	// custom setting for X742 boards
	if (boardInfo->FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
		ret |= CAEN_DGTZ_SetFastTriggerDigitizing(handle,caenCfg->FastTriggerEnabled);
		ret |= CAEN_DGTZ_SetFastTriggerMode(handle,caenCfg->FastTriggerMode);
	}
    if ((boardInfo->FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (boardInfo->FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
        ret |= CAEN_DGTZ_SetDESMode(handle, caenCfg->DesMode);
    }
    ret |= CAEN_DGTZ_SetRecordLength(handle, caenCfg->RecordLength);
    ret |= CAEN_DGTZ_SetPostTriggerSize(handle, caenCfg->PostTrigger);
    ret |= CAEN_DGTZ_SetIOLevel(handle, caenCfg->FPIOtype);
	//ret |= CAEN_DGTZ_SetZeroSuppressionMode(handle, CAEN_DGTZ_ZS_AMP);
	//ret |= CAEN_DGTZ_SetChannelZSParams(handle, 0, CAEN_DGTZ_ZS_FINE, 950, 100);
    //ret |= CAEN_DGTZ_SetAnalogMonOutput(handle, CAEN_DGTZ_AM_BUFFER_OCCUPANCY);
    if( caenCfg->InterruptNumEvents > 0) {
        // Interrupt handling
        if( ret |= CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE,
                                                 VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
                                                 caenCfg->InterruptNumEvents, INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
            printf( "\nError configuring interrupts. Interrupts disabled\n\n");
            caenCfg->InterruptNumEvents = 0;
        }
    }
    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, caenCfg->NumEvents);
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, caenCfg->ExtTriggerMode);

    if ((boardInfo->FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) || (boardInfo->FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE)){
        ret |= CAEN_DGTZ_SetGroupEnableMask(handle, caenCfg->EnableMask);
		for(i=0; i<(caenCfg->NumChannels/8); i++) {
            if (caenCfg->EnableMask & (1<<i)) {
				if (boardInfo->FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
					for(j=0; j<8; j++) {
						if (caenCfg->DCoffsetGrpCh[i][j] != -1)
							ret |= CAEN_DGTZ_SetChannelDCOffset(handle,(i*8)+j, caenCfg->DCoffsetGrpCh[i][j]);
						else
						    ret |= CAEN_DGTZ_SetChannelDCOffset(handle,(i*8)+j, caenCfg->DCoffset[i]);
						}
				}
				else {
					ret |= CAEN_DGTZ_SetGroupDCOffset(handle, i, caenCfg->DCoffset[i]);
					ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle, caenCfg->ChannelTriggerMode[i], (1<<i));
					ret |= CAEN_DGTZ_SetGroupTriggerThreshold(handle, i, caenCfg->Threshold[i]);
					ret |= CAEN_DGTZ_SetChannelGroupMask(handle, i, caenCfg->GroupTrgEnableMask[i]);
				} 
                ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, caenCfg->TriggerEdge);
                
            }
        }
    } else {
        ret |= CAEN_DGTZ_SetChannelEnableMask(handle, caenCfg->EnableMask);
		for(i=0; i<caenCfg->NumChannels; i++) {
            if (caenCfg->EnableMask & (1<<i)) {
                ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, caenCfg->DCoffset[i]);
                ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, caenCfg->ChannelTriggerMode[i], (1<<i));
                ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, caenCfg->Threshold[i]);
                ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, caenCfg->TriggerEdge);
            }
        }
    }
	if (boardInfo->FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
		for(i=0; i<(caenCfg->NumChannels/8); i++) {
			ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset(handle,i,caenCfg->FTDCoffset[i]);
			ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold(handle,i,caenCfg->FTThreshold[i]);
		}
	}
    if (ret)
	{
		textStream<<"Warning: errors found during the programming of the digitizer."<<endl<<"Some settings may not be executed"<<endl;
		return ERR_DGZ_PROGRAM;
	}
	return ERR_NONE;
}

/*! \fn      int ProgramDigitizer(int handle, WaveDumpConfig_t caenCfg)
*   \brief   configure the digitizer according to the parameters read from
*            the cofiguration file and saved in the caenCfg data structure
*
*   \param   handle   Digitizer handle
*   \param   caenCfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/

CAENErrorCode InitDigitizer(int& handle, CAENConfig* caenCfg, CAEN_DGTZ_BoardInfo_t* boardInfo, DataCorrection_t* Table_gr0, DataCorrection_t* Table_gr1, QTextStream& textStream)
{
	/* *************************************************************************************** */
    /* Open the digitizer and read the board information                                       */
    /* *************************************************************************************** */

    int isVMEDevice = caenCfg->BaseAddress ? 1 : 0;
	int MajorNumber;
	CAENComm_ErrorCode ret;
    /* HACK, the function to load the correction table is a CAENComm function, so we first open the
    device with CAENComm lib, read the the correction table and suddenly close the device. */
    if(caenCfg->useCorrections != -1)
	{ 
		// use Corrections Manually
        if (ret = CAENComm_OpenDevice((CAENComm_ConnectionType)caenCfg->LinkType, caenCfg->LinkNum, caenCfg->ConetNode, caenCfg->BaseAddress, &handle)) 
		{
			return ERR_DGZ_OPEN;           
        }
        
        if (ret = (CAENComm_ErrorCode)LoadCorrectionTables(handle, Table_gr0, 0, CAEN_DGTZ_DRS4_2_5GHz))
		{
			return ERR_UNKNOWN;
		}

        if (ret = (CAENComm_ErrorCode)LoadCorrectionTables(handle, Table_gr1, 1, CAEN_DGTZ_DRS4_2_5GHz))
        {
			return ERR_UNKNOWN;
		}

        if (ret = CAENComm_CloseDevice(handle))
        {
			return ERR_UNKNOWN;
		}

        SaveCorrectionTable("table0", Table_gr0, textStream);
        SaveCorrectionTable("table1", Table_gr1, textStream);
        // write tables to file
    }
	CAEN_DGTZ_ErrorCode retDigitizer;
    retDigitizer = CAEN_DGTZ_OpenDigitizer(caenCfg->LinkType, caenCfg->LinkNum, caenCfg->ConetNode, caenCfg->BaseAddress, &handle);

    CAEN_DGTZ_DRS4Frequency_t FREQ;
    retDigitizer = CAEN_DGTZ_GetDRS4SamplingFrequency(handle, &FREQ);

    if(!retDigitizer && caenCfg->useCorrections == -1 )
	{ // use automatic corrections
        retDigitizer = CAEN_DGTZ_LoadDRS4CorrectionData(handle,FREQ);
        retDigitizer = CAEN_DGTZ_EnableDRS4Correction(handle);
    }
    retDigitizer = CAEN_DGTZ_GetInfo(handle, boardInfo);
    if (retDigitizer)
	{
		return ERR_BOARD_INFO_READ;
	}
	textStream<<"Connected to CAEN Digitizer Model "<<boardInfo->ModelName<<endl;
	textStream<<"ROC FPGA Release is "<<boardInfo->ROC_FirmwareRel<<endl;
	textStream<<"AMC FPGA Release is "<<boardInfo->AMC_FirmwareRel<<endl;

    // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
    sscanf(boardInfo->AMC_FirmwareRel, "%d", &MajorNumber);
    if (MajorNumber >= 128) {
		textStream<<"This digitizer has a DPP firmware"<<endl;
		return ERR_INVALID_BOARD_TYPE;
    }

    // get num of channels, num of bit, num of group of the board */
    retDigitizer = GetMoreboardInfo(&handle,boardInfo, caenCfg);
    if (retDigitizer) 
	{
		return ERR_INVALID_BOARD_TYPE;
    }

	// mask the channels not available for this model
    if ((boardInfo->FamilyCode != CAEN_DGTZ_XX740_FAMILY_CODE) && (boardInfo->FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)){
		caenCfg->EnableMask &= (1<<caenCfg->NumChannels)-1;
    } else {
        caenCfg->EnableMask &= (1<<(caenCfg->NumChannels/8))-1;
    }
    if ((boardInfo->FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) && caenCfg->DesMode) {
        caenCfg->EnableMask &= 0xAA;
    }
    if ((boardInfo->FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) && caenCfg->DesMode) {
        caenCfg->EnableMask &= 0x55;
    }    

	return ERR_NONE;
}

CAENErrorCode AllocateEventStorage(int& handle, CAENConfig* caenCfg, CAEN_DGTZ_BoardInfo_t* boardInfo, CAEN_DGTZ_UINT8_EVENT_t *&Event8, CAEN_DGTZ_UINT16_EVENT_t *&Event16, CAEN_DGTZ_X742_EVENT_t *&Event742, char *&buffer, uint32_t& allocatedSize)
{
	// Allocate memory for the event data and readout buffer
	CAEN_DGTZ_ErrorCode ret;
	if(caenCfg->NumBits == 8)
        ret = CAEN_DGTZ_AllocateEvent(handle,(void**)(&Event8));
    else {
		if (boardInfo->FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE) {
			ret = CAEN_DGTZ_AllocateEvent(handle,(void**)(&Event16));
		}
		else {
			ret = CAEN_DGTZ_AllocateEvent(handle,(void**)(&Event742));
		}
    }
    if (ret != CAEN_DGTZ_Success) 
	{
		return ERR_MALLOC;              
    }
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer,&allocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
    if (ret) 
	{
		return ERR_MALLOC;              
    }
	return ERR_NONE;
}


void CloseDigitizer(int handle,  CAEN_DGTZ_UINT8_EVENT_t *Event8, CAEN_DGTZ_UINT16_EVENT_t *Event16, char *buffer)
{
 /* stop the acquisition */
    CAEN_DGTZ_SWStopAcquisition(handle);

    /* close the output files and free histograms*/
    //for(ch=0; ch<WDcfg.Nch; ch++) {
    //    if( WDrun.fout[ch])
    //        fclose(WDrun.fout[ch]);
    //    if( WDrun.Histogram[ch])
    //        free(WDrun.Histogram[ch]);
    //}

    /* close the device and free the buffers */
    if(Event8)
        CAEN_DGTZ_FreeEvent(handle, (void**)&Event8);
    if(Event16)
        CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    CAEN_DGTZ_CloseDigitizer(handle);
}

void SaveCorrectionTable(char *outputFileName, DataCorrection_t* tb, QTextStream& textStream) 
{
    char fnStr[1000];
    int ch,i,j;
    FILE *outputfile;

    strcpy(fnStr, outputFileName);
    strcat(fnStr, "_cell.txt");
	textStream<<"Saving correction table cell values to"<<fnStr<<endl;
    outputfile = fopen(fnStr, "w");
    for(ch=0; ch<MAX_X742_CHANNELS+1; ch++) {
        fprintf(outputfile, "Calibration values from cell 0 to 1024 for channel %d:\n\n", ch);
        for(i=0; i<1024; i+=8) {
            for(j=0; j<8; j++)
                fprintf(outputfile, "%d\t", tb->cell[ch][i+j]);
            fprintf(outputfile, "cell = %d to %d\n", i, i+7);
        }
    }
    fclose(outputfile);

    strcpy(fnStr, outputFileName);
    strcat(fnStr, "_nsample.txt");
	textStream<<"Saving correction table nsamples values to"<<fnStr<<endl;
    outputfile = fopen(fnStr, "w");
    for(ch=0; ch<MAX_X742_CHANNELS+1; ch++) {
        fprintf(outputfile, "Calibration values from cell 0 to 1024 for channel %d:\n\n", ch);
        for(i=0; i<1024; i+=8) {
            for(j=0; j<8; j++)
                fprintf(outputfile, "%d\t", tb->nsample[ch][i+j]);
            fprintf(outputfile, "cell = %d to %d\n", i, i+7);
        }
    }
    fclose(outputfile);

    strcpy(fnStr, outputFileName);
    strcat(fnStr, "_time.txt");
	textStream<<"Saving correction table time values to "<<fnStr<<endl;
    outputfile = fopen(fnStr, "w");
    fprintf(outputfile, "Calibration values (ps) from cell 0 to 1024 :\n\n");
    for(i=0; i<1024; i+=8) {
        for(ch=0; ch<8; ch++)
            fprintf(outputfile, "%09.3f\t", tb->time[i+ch]);
        fprintf(outputfile, "cell = %d to %d\n", i, i+7);
    }
    fclose(outputfile);
}


bool WriteDataHeader(FILE* file, DataHeader* header)
{
	if (fwrite(header, sizeof(DataHeader), 1, file)==1)
	{
		//fflush(file);
		return true;
	}		
	else
		return false;
}

bool WriteDataHeaderCompressed(gzFile file, DataHeader* header)
{
	if (!gzwrite(file, header, sizeof(DataHeader)))
		return false;
	return true;
}

bool AppendEvent(FILE* file, Event* ev)
{
	if (fwrite(&ev->info, sizeof(CAEN_DGTZ_EventInfo_t), 1, file)!=1)
		return false;
	if (fwrite(ev->data.ChSize, sizeof(uint32_t), MAX_UINT16_CHANNEL_SIZE, file)!=MAX_UINT16_CHANNEL_SIZE)
		return false;
	for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
	{
		if (ev->data.ChSize[i] && fwrite(ev->data.DataChannel[i], sizeof(uint16_t), ev->data.ChSize[i], file)!=ev->data.ChSize[i])
			return false;
	}
	return true;
}

bool AppendEventCompressed(gzFile file, Event* ev)
{
	if (!gzwrite(file, &ev->info, sizeof(CAEN_DGTZ_EventInfo_t)))
		return false;
	if (!gzwrite(file, ev->data.ChSize, sizeof(uint32_t)*MAX_UINT16_CHANNEL_SIZE))
		return false;
	for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
	{
		if (ev->data.ChSize[i] && !gzwrite(file, ev->data.DataChannel[i], sizeof(uint16_t) * ev->data.ChSize[i]))
			return false; 
	}
	return true;
}

bool AppendEvent742(FILE* file, Event742* ev)
{
	if (fwrite(&ev->info, sizeof(CAEN_DGTZ_EventInfo_t), 1, file)!=1)
		return false;
    if (fwrite(ev->data.GrPresent, sizeof(uint8_t), MAX_X742_GROUP_SIZE, file)!=MAX_X742_GROUP_SIZE)
		return false;

	for (int i=0;i<MAX_X742_GROUP_SIZE;i++)
	{
		if (ev->data.GrPresent[i])
		{
			if (fwrite(ev->data.DataGroup[i].ChSize, sizeof(uint32_t), MAX_X742_CHANNEL_SIZE, file)!=MAX_X742_CHANNEL_SIZE)
				return false;
			for (int j=0;j<MAX_X742_CHANNEL_SIZE;j++)
			{
				if (ev->data.DataGroup[i].ChSize[j] && fwrite(ev->data.DataGroup[i].DataChannel[j], sizeof(float), ev->data.DataGroup[i].ChSize[j], file)!=ev->data.DataGroup[i].ChSize[j])
					return false;
			}
		}

	}
	return true;
}

bool AppendEvent742Compressed(gzFile file, Event742* ev)
{	
	if (!gzwrite(file, &ev->info, sizeof(CAEN_DGTZ_EventInfo_t)))
		return false;
	if (!gzwrite(file, ev->data.GrPresent, sizeof(uint8_t)*MAX_X742_GROUP_SIZE))
		return false;
	for (int i=0;i<MAX_X742_GROUP_SIZE;i++)
	{
		if (ev->data.GrPresent[i])
		{
			if (!gzwrite(file, ev->data.DataGroup[i].ChSize, sizeof(uint32_t)*MAX_X742_CHANNEL_SIZE))
				return false;
			for (int j=0;j<MAX_X742_CHANNEL_SIZE;j++)
			{
				if (ev->data.DataGroup[i].ChSize[j] && !gzwrite(file, ev->data.DataGroup[i].DataChannel[j], sizeof(float)*ev->data.DataGroup[i].ChSize[j]))
					return false;
			}
		}
	}
	return true;
}

bool AppendEventStatisticsCompressed(gzFile file, EventStatistics* statistics)
{
    if (!gzwrite(file, statistics, sizeof(EventStatistics)))
        return false;
    return true;
}

bool AppendEventStatistics(FILE* file, EventStatistics* statistics)
{
    if (fwrite(statistics, sizeof(EventStatistics), 1, file)!=1)
        return false;
    return true;
}


bool UpdateEventHeader(FILE* file, DataHeader* header)
{
	fpos_t position;
	fgetpos (file, &position);
	rewind(file);
	if (WriteDataHeader(file, header))
	{
		fsetpos(file, &position);
		return true;
	}
	else
		return false;
}

DataHeader::DataHeader()
{
	int x=sizeof(DataHeader);
	memset(this, 0, sizeof(DataHeader));
	time(&dateTime);
	sprintf(description, "Test output.");
	sprintf(futureUse, "RSV");
	sprintf(magicNumber, "UCT001B");
	MSPS=4000;
	numEvents=2;
	numSamples=1024;
	offsetDC=1000;
	peakSaturation=1;
}

Event* Event::eventFromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_UINT16_EVENT_t* data)
{
	Event* ev=new Event();	
	memcpy(&(ev->info), &(info), sizeof(CAEN_DGTZ_EventInfo_t));
	memcpy(ev->data.ChSize, data->ChSize, MAX_UINT16_CHANNEL_SIZE*sizeof(uint16_t));

	for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
	{
		int numSamples=data->ChSize[i];
		if (numSamples)
		{
			ev->data.DataChannel[i]=new uint16_t[numSamples];
			memcpy(ev->data.DataChannel[i], data->DataChannel[i], numSamples*sizeof(uint16_t));
		}
	}
	return ev;
}

void freeEvent(Event* &ev)
{
	for (int i=0;i<MAX_UINT16_CHANNEL_SIZE;i++)
	{
		if (ev->data.ChSize[i])
			delete[] ev->data.DataChannel[i];
	}
	delete ev;
	ev=NULL;
}


Event742* Event742::event742FromInfoAndData(CAEN_DGTZ_EventInfo_t& info, CAEN_DGTZ_X742_EVENT_t* data, uint32_t chMask)
{
	Event742* ev=new Event742();	
	memcpy(&(ev->info), &(info), sizeof(CAEN_DGTZ_EventInfo_t));	
	memcpy(ev->data.GrPresent, data->GrPresent, MAX_X742_GROUP_SIZE*sizeof(uint8_t));

	for (int i=0;i<MAX_X742_GROUP_SIZE;i++)
	{
		if (ev->data.GrPresent[i])
		{
			
			ev->data.DataGroup[i].StartIndexCell=data->DataGroup[i].StartIndexCell;
			ev->data.DataGroup[i].TriggerTimeTag=data->DataGroup[i].TriggerTimeTag;
			memcpy(ev->data.DataGroup[i].ChSize, data->DataGroup[i].ChSize, MAX_X742_CHANNEL_SIZE*sizeof(uint32_t));
			for (int j=0;j<MAX_X742_CHANNEL_SIZE;j++)
            {
				//remove empty channels
                if (!(chMask&1<<j))
				{
					ev->data.DataGroup[i].ChSize[j]=0;
					continue;
				}

				int numSamples=ev->data.DataGroup[i].ChSize[j];
				if (numSamples)
				{
					ev->data.DataGroup[i].DataChannel[j]=new float[numSamples];					
					memcpy(ev->data.DataGroup[i].DataChannel[j], data->DataGroup[i].DataChannel[j], numSamples*sizeof(float));
				}
			}
		}
		
	}
	return ev;
}

void freeEvent742(Event742* &ev)
{
	for (int i=0;i<MAX_X742_GROUP_SIZE;i++)
	{
		if (ev->data.GrPresent[i])
		{			
			for (int j=0;j<MAX_X742_CHANNEL_SIZE;j++)
			{
				if (ev->data.DataGroup[i].ChSize[j])
					delete [] ev->data.DataGroup[i].DataChannel[j];
			}
		}
	}	
	delete ev;
	ev=NULL;
}
