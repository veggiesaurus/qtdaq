/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
* -----------------------------------------------------------------------------
* WDconfig contains the functions for reading the configuration file and 
* setting the parameters in the caenCfg structure
******************************************************************************/


#include <CAEN/CAENDigitizer.h>
#include "CAEN/CAENConfig.h"
#include <QTextStream>


//Default Constructor (CAENConfig)

CAENConfig::CAENConfig()
{
	/* Default settings */
	RecordLength = (1024*16);
	PostTrigger = 80;
	NumEvents = 1023;
	EnableMask = 0xFF;
	GWn = 0;
    ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
    InterruptNumEvents = 0;
    TestPattern = 0;
    TriggerEdge = CAEN_DGTZ_TriggerOnRisingEdge;
    DesMode = CAEN_DGTZ_DISABLE;
	FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED; 
    FastTriggerEnabled = CAEN_DGTZ_DISABLE; 
    FPIOtype = CAEN_DGTZ_IOLevel_NIM;
    GainMultiplier=1;
    ChMask=0;
	for(int i=0; i<MAX_SET; i++) 
	{
		DCoffset[i] = 0;
		Threshold[i] = 0;
        ChannelTriggerMode[i] = CAEN_DGTZ_TRGMODE_DISABLED;
		GroupTrgEnableMask[i] = 0;
		for(int j=0; j<MAX_SET; j++)	
            DCoffsetGrpCh[i][j] = -1;
		FTThreshold[i] = 0;
		FTDCoffset[i] =0;
    }
    useCorrections = -1;
}



/*! \fn      int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *caenCfg) 
*   \brief   Read the configuration file and set the WaveDump paremeters
*            
*   \param   f_ini        Pointer to the config file
*   \param   caenCfg:   Pointer to the CAENConfig data structure
*   \param   textStream: Pointer to the text stream to log to
*   \return  0 = Success; negative numbers are error codes
*/
CAENErrorCode ParseConfigFile(const char* fileName, CAENConfig *caenCfg, QTextStream& textStream)
{	
	FILE *f_ini=fopen(fileName, "r");
	if (!f_ini)
		return ERR_CONF_FILE_NOT_FOUND;
	char str[1000], str1[1000];
	int i,j, ch=-1, val, Off=0, tr = -1;

	textStream<<"Reading config file"<<endl;
	/* read config file and assign parameters */
	while(!feof(f_ini)) {
		int read;
        // read a word from the file
        read = fscanf(f_ini, "%s", str);
        if( !read || (read == EOF) || !strlen(str))
			continue;
        // skip comments
        if(str[0] == '#') {
            fgets(str, 1000, f_ini);
			continue;
        }

        if (strcmp(str, "@ON")==0) {
            Off = 0;
            continue;
        }
		if (strcmp(str, "@OFF")==0)
            Off = 1;
        if (Off)
            continue;


        // Section (COMMON or individual channel)
		if (str[0] == '[') {
            if (strstr(str, "COMMON")) {
                ch = -1;
               continue; 
            }
            if (strstr(str, "TR")) {
				sscanf(str+1, "TR%d", &val);
				 if (val < 0 || val >= MAX_SET) 
				 {
					 textStream<<str<<": Invalid channel number"<<endl;
					fclose(f_ini);
					 return ERR_CONF_FILE_INVALID;
                } else {
                    tr = val;
                }
            } else {
                sscanf(str+1, "%d", &val);
                if (val < 0 || val >= MAX_SET) 
				{
					textStream<<str<<": Invalid channel number"<<endl;
					fclose(f_ini);
					return ERR_CONF_FILE_INVALID;
                } else {
                    ch = val;
                }
            }
            continue;
		}
 
        // OPEN: read the details of physical path to the digitizer
		if (strstr(str, "OPEN")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "USB")==0)
				caenCfg->LinkType = CAEN_DGTZ_USB;			
			else if (strcmp(str1, "PCIE")==0)
				caenCfg->LinkType = CAEN_DGTZ_PCIE_OpticalLink;
			else if (strcmp(str1, "PCI")==0)
				caenCfg->LinkType = CAEN_DGTZ_PCI_OpticalLink;
            else 
			{
				textStream<<str<<" "<<str1<<": Invalid connection type"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
            }
			fscanf(f_ini, "%d", &caenCfg->LinkNum);
            if (caenCfg->LinkType == CAEN_DGTZ_USB)
                caenCfg->ConetNode = 0;
            else
			    fscanf(f_ini, "%d", &caenCfg->ConetNode);
			fscanf(f_ini, "%x", &caenCfg->BaseAddress);
			continue;
		}

        if (strstr(str, "NUM_BASELINE_SAMPLES")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->NumBaselineSamples);
            continue;
        }

        if (strstr(str, "PEAK_MIN_ADC_COUNT")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->PeakMinAdcCount);
            continue;
        }

        if (strstr(str, "SAMPLES_PER_MICROSECOND")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->SamplesPerMicrosecond);
            continue;
        }

        if (strstr(str, "LOW_PASS_ALPHA")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->LowPassAlpha);
            continue;
        }

        if (strstr(str, "TRIGGER_MIN_LENGTH")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->TriggerMinLength);
            continue;
        }

        if (strstr(str, "DELTA_TRIGGER_THRESHOLD")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->DeltaTriggerThreshold);
            continue;
        }

        if (strstr(str, "GAIN_MULTIPLIER")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->GainMultiplier);
            continue;
        }

        if (strstr(str, "DELTA_TRIGGER_SECOND_PEAK_THRESHOLD")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->DeltaTriggerThresholdSecondPeak);
            continue;
        }

        if (strstr(str, "TRIGGER_SEARCH_RANGE")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->TriggerSearchRange);
            continue;
        }

        if (strstr(str, "INTEGRAL_START_OFFSET_NS")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->IntegralStartOffset);
            continue;
        }

        if (strstr(str, "INTEGRAL_SHORT_GATE_OFFSET_NS")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->IntegralShortGateOffset);
            continue;
        }

        if (strstr(str, "INTEGRAL_LONG_GATE_OFFSET_NS")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->IntegralLongGateOffset);
            continue;
        }

        if (strstr(str, "NUM_CACHED_EVENTS")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->NumCachedEvents);
            continue;
        }
        if (strstr(str, "DISPLAY_SIGNAL")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->DisplaySignal);
            continue;
        }
        if (strstr(str, "DELAY_DISPLAY")!=NULL) {
            fscanf(f_ini, "%d", &caenCfg->DisplaySignalDelay);
            continue;
        }

		// Generic VME Write (address offset + data, both exadecimal)
		if ((strstr(str, "WRITE_REGISTER")!=NULL) && (caenCfg->GWn < MAX_GW)) {
			fscanf(f_ini, "%x", (int *)&caenCfg->GWaddr[caenCfg->GWn]);
			fscanf(f_ini, "%x", (int *)&caenCfg->GWdata[caenCfg->GWn]);
			caenCfg->GWn++;
			continue;
		}

        // Acquisition Record Length (number of samples)
		if (strstr(str, "RECORD_LENGTH")!=NULL) {
			fscanf(f_ini, "%d", &caenCfg->RecordLength);
			continue;
		}

        // Correction Level (mask)
		if (strstr(str, "CORRECTION_LEVEL")!=NULL) {
			fscanf(f_ini, "%s", str1);
            if( strcmp(str1, "AUTO") == 0 )
                caenCfg->useCorrections = -1;
            else
                caenCfg->useCorrections = atoi(str1);
			continue;
		}

        // Test Pattern
		if (strstr(str, "TEST_PATTERN")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES")==0)
				caenCfg->TestPattern = 1;
			else if (strcmp(str1, "NO")!=0)
			{
				textStream<<str<<": Invalid option"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
			continue;
		}

        // Trigger Edge
		if (strstr(str, "TRIGGER_EDGE")!=NULL) 
		{
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "FALLING")==0)
				caenCfg->TriggerEdge = CAEN_DGTZ_TriggerOnFallingEdge;
			else if (strcmp(str1, "RISING")!=0)
			{
				textStream<<str<<": Invalid operation"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
			continue;
		}

        // External Trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
		if (strstr(str, "EXTERNAL_TRIGGER")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED")==0)
                caenCfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                caenCfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else if (strcmp(str1, "ACQUISITION_AND_TRGOUT")==0)
                caenCfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
            else
			{
				textStream<<str<<": Invalid parameter"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
            continue;
		}

        // Max. number of events for a block transfer (0 to 1023)
		if (strstr(str, "MAX_NUM_EVENTS_BLT")!=NULL) 
		{
			fscanf(f_ini, "%d", &caenCfg->NumEvents);
			continue;
		}

		
		// Post Trigger (percent of the acquisition window)
		if (strstr(str, "POST_TRIGGER")!=NULL) 
		{
			fscanf(f_ini, "%d", &caenCfg->PostTrigger);
			continue;
		}

        // DesMode (Double sampling frequency for the Mod 731 and 751)
		if (strstr(str, "ENABLE_DES_MODE")!=NULL) 
		{
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES")==0)
				caenCfg->DesMode = CAEN_DGTZ_ENABLE;
			else if (strcmp(str1, "NO")!=0)
			{
				textStream<<str<<": Invalid option"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
			continue;
		}

		// Output file format (BINARY or ASCII)
		if (strstr(str, "OUTPUT_FILE_FORMAT")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "BINARY")==0)
				caenCfg->OutFileFlags= (caenCfg->OutFileFlags|BinaryOutput);
			else if (strcmp(str1, "ASCII")!=0)
			{
				textStream<<str1<<": Invalid output format"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
			continue;
		}

		// Header into output file (YES or NO)
		if (strstr(str, "OUTPUT_FILE_HEADER")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES")==0)
				caenCfg->OutFileFlags= (caenCfg->OutFileFlags|HeaderOutput);
			else if (strcmp(str1, "NO")!=0)
			{
				textStream<<str<<": Invalid option"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
			continue;
		}

        // Interrupt settings (request interrupt when there are at least N events to read; 0=disable interrupts (polling mode))
		if (strstr(str, "USE_INTERRUPT")!=NULL) {
			fscanf(f_ini, "%d", &caenCfg->InterruptNumEvents);
			continue;
		}
		
		if (!strcmp(str, "FAST_TRIGGER")) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED")==0)
                caenCfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                caenCfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
            else
			{
				textStream<<str<<": Invalid parameter"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
            continue;
		}



		if (strstr(str, "ENABLED_FAST_TRIGGER_DIGITIZING")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES")==0)
				caenCfg->FastTriggerEnabled= CAEN_DGTZ_ENABLE;
			else if (strcmp(str1, "NO")!=0)
			{
				textStream<<str<<": Invalid option"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
			}
			continue;
		}
		
		// Channel Flags
		if (strstr(str, "CH_ENABLE_MASK")!=NULL) {
            fscanf(f_ini, "%x", (int*)&caenCfg->ChMask);
			continue;
		}


		// DC offset (percent of the dynamic range, -50 to 50)
		if (!strcmp(str, "DC_OFFSET")) {
            float dc;
			fscanf(f_ini, "%f", &dc);
			if (tr != -1) {
// 				caenCfg->FTDCoffset[tr] = dc;
 				caenCfg->FTDCoffset[tr*2] = (uint32_t)dc;
 				caenCfg->FTDCoffset[tr*2+1] = (uint32_t)dc;
				continue;
			}
            val = (int)((dc+50) * 65535 / 100);
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    caenCfg->DCoffset[i] = val;
            else
                caenCfg->DCoffset[ch] = val;
			continue;
		}
		
		if (strstr(str, "GRP_CH_DC_OFFSET")!=NULL) {
            float dc[8];
			fscanf(f_ini, "%f,%f,%f,%f,%f,%f,%f,%f", &dc[0], &dc[1], &dc[2], &dc[3], &dc[4], &dc[5], &dc[6], &dc[7]);
            for(i=0; i<MAX_SET; i++) {
				val = (int)((dc[i]+50) * 65535 / 100); 
				caenCfg->DCoffsetGrpCh[ch][i] = val;
            }
			continue;
		}

		// Threshold
		if (strstr(str, "TRIGGER_THRESHOLD")!=NULL) {
			fscanf(f_ini, "%d", &val);
			if (tr != -1) {
//				caenCfg->FTThreshold[tr] = val;
 				caenCfg->FTThreshold[tr*2] = val;
 				caenCfg->FTThreshold[tr*2+1] = val;

				continue;
			}
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    caenCfg->Threshold[i] = val;
            else
                caenCfg->Threshold[ch] = val;
			continue;
		}

		// Group Trigger Enable Mask (hex 8 bit)
		if (strstr(str, "GROUP_TRG_ENABLE_MASK")!=NULL) {
			fscanf(f_ini, "%x", &val);
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    caenCfg->GroupTrgEnableMask[i] = val & 0xFF;
            else
                 caenCfg->GroupTrgEnableMask[ch] = val & 0xFF;
			continue;
		}

        // Channel Auto trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
		if (strstr(str, "CHANNEL_TRIGGER")!=NULL) {
            CAEN_DGTZ_TriggerMode_t tm;
			fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "DISABLED")==0)
                tm = CAEN_DGTZ_TRGMODE_DISABLED;
            else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else if (strcmp(str1, "ACQUISITION_AND_TRGOUT")==0)
                tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
            else 
			{
				textStream<<str<<": Invalid parameter"<<endl;                
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
                continue;
            }
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    caenCfg->ChannelTriggerMode[i] = tm;
            else
                caenCfg->ChannelTriggerMode[ch] = tm;
		    continue;
		}

        // Front Panel LEMO I/O level (NIM, TTL)
		if (strstr(str, "FPIO_LEVEL")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "TTL")==0)
				caenCfg->FPIOtype = CAEN_DGTZ_IOLevel_TTL;
			else if (strcmp(str1, "NIM")!=0)
			{
				textStream<<str<<": Invalid option"<<endl;
				fclose(f_ini);				
				return ERR_CONF_FILE_INVALID;                
				//printf("%s: invalid option\n", str);
			}
			continue;
		}

        // Channel Enable (or Group enable for the V1740) (YES/NO)
        if (strstr(str, "ENABLE_INPUT")!=NULL) {
			fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0) {
                if (ch == -1)
                    caenCfg->EnableMask = 0xFF;
                else
                    caenCfg->EnableMask |= (1 << ch);
			    continue;
            } else if (strcmp(str1, "NO")==0) {
                if (ch == -1)
                    caenCfg->EnableMask = 0x00;
                else
                    caenCfg->EnableMask &= ~(1 << ch);
			    continue;
            } else 
			{
				textStream<<str<<": Invalid option"<<endl;
				fclose(f_ini);
				return ERR_CONF_FILE_INVALID;                
            }
			continue;
		}

		textStream<<str<<": Invalid setting"<<endl;
		fclose(f_ini);
		return ERR_CONF_FILE_INVALID;                
	}

	//finished reading successfully
	fclose(f_ini);
	textStream<<"Finished reading file"<<endl;
	return ERR_NONE;
}

