#include "VxAcquisitionConfig.h"

VxAcquisitionConfig::VxAcquisitionConfig()
{
	RecordLength = 4096;
	PostTrigger = 80;
	NumEvents = 512;
	EnableMask = 0xFFFF;
	GWn = 0;
	ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
	InterruptNumEvents = 0;
	TestPattern = false;
	TriggerEdge = CAEN_DGTZ_TriggerOnRisingEdge;
	DesMode = 0;
	FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
	FastTriggerEnabled = 0;
	FPIOtype = CAEN_DGTZ_IOLevel_NIM;

	for (int i = 0; i < MAX_SET; i++) {
		DCoffset[i] = 0;
		Threshold[i] = 0;
		ChannelTriggerMode[i] = CAEN_DGTZ_TRGMODE_DISABLED;		
		FTThreshold[i] = 0;
		FTDCoffset[i] = 0;
	}
}

VxAcquisitionConfig* VxAcquisitionConfig::parseConfigString(QString configString, int& errorLineNumber, QString& errorString)
{
	VxAcquisitionConfig* config = new VxAcquisitionConfig();

	char str[1000], str1[1000], *pread;
	int i, ch = -1, val, Off = 0, tr = -1;
	int ret = 0;

	/* read config file and assign parameters */
	while (!feof(f_ini)) {
		int read;
		char *res;
		// read a word from the file
		read = fscanf(f_ini, "%s", str);
		if (!read || (read == EOF) || !strlen(str))
			continue;
		// skip comments
		if (str[0] == '#') {
			res = fgets(str, 1000, f_ini);
			continue;
		}

		if (strcmp(str, "@ON") == 0) {
			Off = 0;
			continue;
		}
		if (strcmp(str, "@OFF") == 0)
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
				sscanf(str + 1, "TR%d", &val);
				if (val < 0 || val >= MAX_SET) {
					printf("%s: Invalid channel number\n", str);
				}
				else {
					tr = val;
				}
			}
			else {
				sscanf(str + 1, "%d", &val);
				if (val < 0 || val >= MAX_SET) {
					printf("%s: Invalid channel number\n", str);
				}
				else {
					ch = val;
				}
			}
			continue;
		}

		// OPEN: read the details of physical path to the digitizer
		if (strstr(str, "OPEN") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "USB") == 0)
				config->LinkType = CAEN_DGTZ_USB;
			else if (strcmp(str1, "PCI") == 0)
				config->LinkType = CAEN_DGTZ_OpticalLink;
			else {
				printf("%s %s: Invalid connection type\n", str, str1);
				return -1;
			}
			read = fscanf(f_ini, "%d", &config->LinkNum);
			if (config->LinkType == CAEN_DGTZ_USB)
				config->ConetNode = 0;
			else
				read = fscanf(f_ini, "%d", &config->ConetNode);
			read = fscanf(f_ini, "%x", &config->BaseAddress);
			continue;
		}

		// Generic VME Write (address offset + data + mask, each exadecimal)
		if ((strstr(str, "WRITE_REGISTER") != NULL) && (config->GWn < MAX_GW)) {
			read = fscanf(f_ini, "%x", (int *)&config->GWaddr[config->GWn]);
			read = fscanf(f_ini, "%x", (int *)&config->GWdata[config->GWn]);
			read = fscanf(f_ini, "%x", (int *)&config->GWmask[config->GWn]);
			config->GWn++;
			continue;
		}

		// Acquisition Record Length (number of samples)
		if (strstr(str, "RECORD_LENGTH") != NULL) {
			read = fscanf(f_ini, "%d", &config->RecordLength);
			continue;
		}
		
		// Test Pattern
		if (strstr(str, "TEST_PATTERN") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES") == 0)
				config->TestPattern = 1;
			else if (strcmp(str1, "NO") != 0)
				printf("%s: invalid option\n", str);
			continue;
		}

		// Trigger Edge
		if (strstr(str, "TRIGGER_EDGE") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "FALLING") == 0)
				config->TriggerEdge = 1;
			else if (strcmp(str1, "RISING") != 0)
				printf("%s: invalid option\n", str);
			continue;
		}

		// External Trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
		if (strstr(str, "EXTERNAL_TRIGGER") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED") == 0)
				config->ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY") == 0)
				config->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else if (strcmp(str1, "ACQUISITION_AND_TRGOUT") == 0)
				config->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
			else
				printf("%s: Invalid Parameter\n", str);
			continue;
		}

		// Max. number of events for a block transfer (0 to 1023)
		if (strstr(str, "MAX_NUM_EVENTS_BLT") != NULL) {
			read = fscanf(f_ini, "%d", &config->NumEvents);
			continue;
		}

		// Post Trigger (percent of the acquisition window)
		if (strstr(str, "POST_TRIGGER") != NULL) {
			read = fscanf(f_ini, "%d", &config->PostTrigger);
			continue;
		}

		// DesMode (Double sampling frequency for the Mod 731 and 751)
		if (strstr(str, "ENABLE_DES_MODE") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES") == 0)
				config->DesMode = 1;
			else if (strcmp(str1, "NO") != 0)
				printf("%s: invalid option\n", str);			
			continue;
		}		

		// Interrupt settings (request interrupt when there are at least N events to read; 0=disable interrupts (polling mode))
		if (strstr(str, "USE_INTERRUPT") != NULL) {
			read = fscanf(f_ini, "%d", &config->InterruptNumEvents);
			continue;
		}

		if (!strcmp(str, "FAST_TRIGGER")) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED") == 0)
				config->FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY") == 0)
				config->FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else
				printf("%s: Invalid Parameter\n", str);
			continue;
		}

		if (strstr(str, "ENABLED_FAST_TRIGGER_DIGITIZING") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES") == 0)
				config->FastTriggerEnabled = 1;
			else if (strcmp(str1, "NO") != 0)
				printf("%s: invalid option\n", str);
			continue;
		}

		// DC offset (percent of the dynamic range, -50 to 50)
		if (!strcmp(str, "DC_OFFSET")) {
			float dc;
			read = fscanf(f_ini, "%f", &dc);
			if (tr != -1) {
				// 				config->FTDCoffset[tr] = dc;
				config->FTDCoffset[tr * 2] = (uint32_t)dc;
				config->FTDCoffset[tr * 2 + 1] = (uint32_t)dc;
				continue;
			}
			val = (int)((dc + 50) * 65535 / 100);
			if (ch == -1)
				for (i = 0; i < MAX_SET; i++)
					config->DCoffset[i] = val;
			else
				config->DCoffset[ch] = val;
			continue;
		}

		// Threshold
		if (strstr(str, "TRIGGER_THRESHOLD") != NULL) {
			read = fscanf(f_ini, "%d", &val);
			if (tr != -1) {
				//				config->FTThreshold[tr] = val;
				config->FTThreshold[tr * 2] = val;
				config->FTThreshold[tr * 2 + 1] = val;

				continue;
			}
			if (ch == -1)
				for (i = 0; i < MAX_SET; i++)
					config->Threshold[i] = val;
			else
				config->Threshold[ch] = val;
			continue;
		}
				
		// Channel Auto trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
		if (strstr(str, "CHANNEL_TRIGGER") != NULL) {
			CAEN_DGTZ_TriggerMode_t tm;
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED") == 0)
				tm = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY") == 0)
				tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else if (strcmp(str1, "ACQUISITION_AND_TRGOUT") == 0)
				tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
			else {
				printf("%s: Invalid Parameter\n", str);
				continue;
			}
			if (ch == -1)
				for (i = 0; i < MAX_SET; i++)
					config->ChannelTriggerMode[i] = tm;
			else
				config->ChannelTriggerMode[ch] = tm;
			continue;
		}

		// Front Panel LEMO I/O level (NIM, TTL)
		if (strstr(str, "FPIO_LEVEL") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "TTL") == 0)
				config->FPIOtype = CAEN_DGTZ_IOLevel_TTL;
			else if (strcmp(str1, "NIM") != 0)
				printf("%s: invalid option\n", str);
			continue;
		}

		// Channel Enable (or Group enable for the V1740) (YES/NO)
		if (strstr(str, "ENABLE_INPUT") != NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "YES") == 0) {
				if (ch == -1)
					config->EnableMask = 0xFF;
				else
					config->EnableMask |= (1 << ch);
				continue;
			}
			else if (strcmp(str1, "NO") == 0) {
				if (ch == -1)
					config->EnableMask = 0x00;
				else
					config->EnableMask &= ~(1 << ch);
				continue;
			}
			else {
				printf("%s: invalid option\n", str);
			}
			continue;
		}

		printf("%s: invalid setting\n", str);
	}
	return ret;

	SAFE_DELETE(config);
	return nullptr;
}
