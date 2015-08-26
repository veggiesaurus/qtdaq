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

VxAcquisitionConfig* VxAcquisitionConfig::parseConfigString(QString configString)
{
	QVector<VxParseError> parseErrors;
	return parseConfigString(configString, parseErrors);
}

VxAcquisitionConfig* VxAcquisitionConfig::parseConfigString(QString configString, QVector<VxParseError>& parseErrors)
{
	parseErrors.clear();
	VxAcquisitionConfig* config = new VxAcquisitionConfig();

	QStringList lines = configString.split('\n');
	int lineNum = 0;
	int channelNum = -1;
	for each (auto line in lines)
	{
		lineNum++;
		if (line.trimmed().startsWith('#'))
			continue;
		if (QRegularExpression("\\[\\S+\\]", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			channelNum = -1;
			auto matchSectionPattern = QRegularExpression("\\[(COMMON|\\d+)\\]", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchSectionPattern.hasMatch())
			{
				int newChannelNum = matchSectionPattern.captured(0).contains("COMMON", Qt::CaseInsensitive) ? -1 : matchSectionPattern.captured(0).toInt();
				if (newChannelNum >= -1 && newChannelNum <= MAX_SET)
					channelNum = newChannelNum;
				else
					parseErrors.push_back({ VxParseError::WARNING, lineNum, "Invalid channel section. Defaulting to COMMON" });
			}
			else
				parseErrors.push_back({ VxParseError::WARNING, lineNum, "Invalid section. Defaulting to COMMON"});
		}
		else if (QRegularExpression("OPEN", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchOpenPCI = QRegularExpression("OPEN\\s+(PCI|PCIE)\\s+(\\d+)\\s+(\\d+)\\s+([0-9 A-F]+)", QRegularExpression::CaseInsensitiveOption).match(line);
			auto matchOpenUSB = QRegularExpression("OPEN\\s+USB\\s+(\\d+)\\s+([0-9 A-F]+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchOpenPCI.hasMatch())
			{
				config->LinkType = CAEN_DGTZ_OpticalLink;
				config->LinkNum = matchOpenPCI.captured(3).toInt();
				config->ConetNode = matchOpenPCI.captured(5).toInt();
				config->BaseAddress = matchOpenPCI.captured(7).toInt(0, 16);
			}
			else if (matchOpenUSB.hasMatch())
			{
				config->LinkType = CAEN_DGTZ_USB;
				config->LinkNum = matchOpenPCI.captured(2).toInt();
				config->BaseAddress = matchOpenPCI.captured(4).toInt(0, 16);
			}
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid OPEN command. OPTIONS ARE PCI/PCIE/USB, followed by connection parameters" });
		}
		else if (QRegularExpression("RECORD_LENGTH", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchRecordLength = QRegularExpression("RECORD_LENGTH\\s+(\\d+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchRecordLength.hasMatch())
				config->RecordLength = matchRecordLength.captured(1).toInt();
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid RECORD_LENGTH command. Format is 'RECORD_LENGTH #N', where #N is the number of samples" });
		}
		else if (QRegularExpression("TEST_PATTERN", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchTestPattern = QRegularExpression("TEST_PATTERN\\s+(YES|NO)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchTestPattern.hasMatch())
				config->TestPattern = (matchTestPattern.captured(1).contains("YES", Qt::CaseInsensitive));
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid TEST_PATTERN command. Format is 'TEST_PATTERN YES|NO'" });
		}
		else if (QRegularExpression("ENABLE_DES_MODE", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchDesModePattern = QRegularExpression("ENABLE_DES_MODE\\s+(YES|NO)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchDesModePattern.hasMatch())
				config->DesMode = (matchDesModePattern.captured(1).contains("YES", Qt::CaseInsensitive));
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid ENABLE_DES_MODE command. Format is 'ENABLE_DES_MODE YES|NO'" });
		}
		else if (QRegularExpression("EXTERNAL_TRIGGER", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchExternalTriggerPattern = QRegularExpression("EXTERNAL_TRIGGER\\s+(DISABLED|ACQUISITION_ONLY|ACQUISITION_AND_TRGOUT)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchExternalTriggerPattern.hasMatch())
				config->ExtTriggerMode = (matchExternalTriggerPattern.captured(1).contains("ACQUISITION_ONLY", Qt::CaseInsensitive)) ? CAEN_DGTZ_TRGMODE_ACQ_ONLY : ((matchExternalTriggerPattern.captured(0).contains("ACQUISITION_AND_TRGOUT", Qt::CaseInsensitive)) ? CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT : CAEN_DGTZ_TRGMODE_DISABLED);
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid EXTERNAL_TRIGGER command. Format is 'EXTERNAL_TRIGGER DISABLED|ACQUISITION_ONLY|ACQUISITION_AND_TRGOUT'" });
		}
		else if (QRegularExpression("MAX_NUM_EVENTS_BLT", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchBLTPattern = QRegularExpression("MAX_NUM_EVENTS_BLT\\s+(\\d+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchBLTPattern.hasMatch())
			{
				int numEventsBLT = matchBLTPattern.captured(1).toInt();
				config->NumEvents = 1;
				if (numEventsBLT >= 1 && numEventsBLT <= 1023)
					config->NumEvents = numEventsBLT;
				else
					parseErrors.push_back({ VxParseError::WARNING, lineNum, "MAX_NUM_EVENTS_BLT must be between 1 and 1023. Defaulting to 1" });
			}
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid MAX_NUM_EVENTS_BLT command. Format is 'MAX_NUM_EVENTS_BLT #N', where #N is the number of events to read out in one block transfer" });
		}
		else if (QRegularExpression("POST_TRIGGER", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchPostTriggerPattern = QRegularExpression("POST_TRIGGER\\s+(\\d+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchPostTriggerPattern.hasMatch())
			{
				int postTriggerDelay = matchPostTriggerPattern.captured(1).toInt();
				config->PostTrigger = 0;
				if (postTriggerDelay >= 0 && postTriggerDelay <= 100)
					config->PostTrigger = postTriggerDelay;
				else
					parseErrors.push_back({ VxParseError::WARNING, lineNum, "POST_TRIGGER must be between 0 and 100. Defaulting to 1" });
			}
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid POST_TRIGGER command. Format is 'POST_TRIGGER #N', where #N is the percentage of the acquisition window" });
		}
		else if (QRegularExpression("TRIGGER_EDGE", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchTriggerEdgePattern = QRegularExpression("TRIGGER_EDGE\\s+(RISING|FALLING)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchTriggerEdgePattern.hasMatch())
				config->TriggerEdge = (matchTriggerEdgePattern.captured(1).contains("RISING", Qt::CaseInsensitive)) ? CAEN_DGTZ_TriggerOnRisingEdge : CAEN_DGTZ_TriggerOnFallingEdge;
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid TRIGGER_EDGE command. Format is 'TRIGGER_EDGE RISING|FALLING'" });
		}
		else if (QRegularExpression("USE_INTERRUPT", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchInterruptsPattern = QRegularExpression("USE_INTERRUPT\\s+(\\d+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchInterruptsPattern.hasMatch())
			{
				int numEventsBLT = matchInterruptsPattern.captured(1).toInt();
				config->InterruptNumEvents = 0;
				if (numEventsBLT >= 0 && numEventsBLT <= 1023)
					config->InterruptNumEvents = numEventsBLT;
				else
					parseErrors.push_back({ VxParseError::WARNING, lineNum, "USE_INTERRUPT must be between 0 and 1023. Defaulting to 0" });
			}
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid USE_INTERRUPT command. Format is 'USE_INTERRUPT #N', where #N is the number of events before asserting IRQ" });
		}
		else if (QRegularExpression("FPIO_LEVEL", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchFpioLevelPattern = QRegularExpression("FPIO_LEVEL\\s+(NIM|TTL)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchFpioLevelPattern.hasMatch())
				config->FPIOtype = (matchFpioLevelPattern.captured(1).contains("NIM", Qt::CaseInsensitive)) ? CAEN_DGTZ_IOLevel_NIM : CAEN_DGTZ_IOLevel_TTL;
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid FPIO_LEVEL command. Format is 'FPIO_LEVEL NIM|TTL'" });
		}
		else if (QRegularExpression("WRITE_REGISTER", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchWriteRegisterPattern = QRegularExpression("WRITE_REGISTER\\s+([0-9 A-F]+)\\s+([0-9 A-F]+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchWriteRegisterPattern.hasMatch())
			{
				int writeAddress = matchWriteRegisterPattern.captured(1).toInt(0, 16);
				int writeData = matchWriteRegisterPattern.captured(3).toInt(0, 16);
				config->GWaddr[config->GWn] = writeAddress;
				config->GWdata[config->GWn] = writeData;
				config->GWn++;
			}
			else
				parseErrors.push_back({ VxParseError::WARNING, lineNum, "Invalid WRITE_REGISTER command. Format is 'WRITE_REGISTER #ADDR #DATA'" });
		}
		else if (QRegularExpression("ENABLE_INPUT", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchEnableInputPattern = QRegularExpression("ENABLE_INPUT\\s+(YES|NO)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchEnableInputPattern.hasMatch() && channelNum >= 0)
			{
				if (matchEnableInputPattern.captured(1).contains("YES", Qt::CaseInsensitive))
					config->EnableMask |= (1 << channelNum);
				else
					config->EnableMask &= ~(1 << channelNum);
			}
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid ENABLE_INPUT command, or invalid channel number. Format is 'ENABLE_INPUT YES|NO'" });
		}
		else if (QRegularExpression("DC_OFFSET", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch() && !QRegularExpression("GRP_CH_DC_OFFSET", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchDcOffsetPattern = QRegularExpression("DC_OFFSET\\s+(-+)?([0-9]*\\.[0-9]+|[0-9]+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchDcOffsetPattern.hasMatch() && channelNum >= 0)
			{
				float floatVal = (matchDcOffsetPattern.captured(1) + matchDcOffsetPattern.captured(2)).toFloat();
				if (floatVal >= -50.0f && floatVal <= 50.0f)
					config->DCoffset[channelNum] = floatVal;
				else
					parseErrors.push_back({ VxParseError::WARNING, lineNum, "Invalid DC_OFFSET command, or invalid channel number. DC_OFFSET must be between -50.0 and 50.0" });

			}
			else
				parseErrors.push_back({ VxParseError::WARNING, lineNum, "Invalid DC_OFFSET command, or invalid channel number. Format is 'DC_OFFSET #F'" });
		}
		else if (QRegularExpression("TRIGGER_THRESHOLD", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchTriggerThresholdPattern = QRegularExpression("TRIGGER_THRESHOLD\\s+(\\d+)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchTriggerThresholdPattern.hasMatch() && channelNum >= 0)
			{
				int triggerThreshold = matchTriggerThresholdPattern.captured(1).toInt();
				config->Threshold[channelNum] = 512;
				if (triggerThreshold >= 0 && triggerThreshold <= 4095)
					config->Threshold[channelNum] = triggerThreshold;
				else
					parseErrors.push_back({ VxParseError::WARNING, lineNum, "TRIGGER_THRESHOLD must be between 0 and 4095. Defaulting to 512" });
			}
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid TRIGGER_THRESHOLD command. Format is 'TRIGGER_THRESHOLD #N', where #N is the trigger threshold in ADC counts" });
		}
		else if (QRegularExpression("CHANNEL_TRIGGER", QRegularExpression::CaseInsensitiveOption).match(line).hasMatch())
		{
			auto matchChannelTriggerPattern = QRegularExpression("CHANNEL_TRIGGER\\s+(DISABLED|ACQUISITION_ONLY|ACQUISITION_AND_TRGOUT)", QRegularExpression::CaseInsensitiveOption).match(line);
			if (matchChannelTriggerPattern.hasMatch() && channelNum >= 0)
				config->ChannelTriggerMode[channelNum] = (matchChannelTriggerPattern.captured(1).contains("ACQUISITION_ONLY", Qt::CaseInsensitive)) ? CAEN_DGTZ_TRGMODE_ACQ_ONLY : ((matchChannelTriggerPattern.captured(0).contains("ACQUISITION_AND_TRGOUT", Qt::CaseInsensitive)) ? CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT : CAEN_DGTZ_TRGMODE_DISABLED);
			else
				parseErrors.push_back({ VxParseError::CRITICAL, lineNum, "Invalid CHANNEL_TRIGGER command. Format is 'CHANNEL_TRIGGER DISABLED|ACQUISITION_ONLY|ACQUISITION_AND_TRGOUT'" });
		}
		else if (QRegularExpression("\\s*\\S+").match(line).hasMatch())
			parseErrors.push_back({ VxParseError::WARNING, lineNum, "Invalid config option" });
	}

	for each (auto parseError in parseErrors)
	{
		if (parseError.errorType == VxParseError::CRITICAL)
		{
			SAFE_DELETE(config);
			return nullptr;
		}
	}
	return config;

	/*
	char str[1000], str1[1000], *pread;
	int i, ch = -1, val, Off = 0, tr = -1;
	int ret = 0;

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
	*/		
}
