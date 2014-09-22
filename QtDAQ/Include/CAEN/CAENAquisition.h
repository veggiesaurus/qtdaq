#pragma once

#include <time.h>
#include <CAEN/CAENDigitizer.h>
#include <CAEN/X742CorrectionRoutines.h>
#include "CAEN/CAENConfig.h"
#include "CAEN/WaveDump.h"
#include "zlib/zlib.h"
#include "AcquisitionDefinitions.h"


CAENErrorCode ProgramDigitizer(int handle, CAENConfig* caenCfg, CAEN_DGTZ_BoardInfo_t* boardInfo, QTextStream& textStream);


CAEN_DGTZ_ErrorCode GetMoreBoardInfo(int& handle, CAEN_DGTZ_BoardInfo_t* boardInfo, CAENConfig* caenCfg);

CAENErrorCode InitDigitizer(int& handle, CAENConfig* caenCfg, CAEN_DGTZ_BoardInfo_t* boardInfo, DataCorrection_t* Table_gr0, DataCorrection_t* Table_gr1, QTextStream& textStream);

void CloseDigitizer(int handle,  CAEN_DGTZ_UINT8_EVENT_t *Event8=NULL, CAEN_DGTZ_UINT16_EVENT_t *Event16=NULL, char *buffer=NULL);

CAENErrorCode AllocateEventStorage(int& handle, CAENConfig* caenCfg, CAEN_DGTZ_BoardInfo_t* boardInfo, CAEN_DGTZ_UINT8_EVENT_t *&Event8, CAEN_DGTZ_UINT16_EVENT_t *&Event16, CAEN_DGTZ_X742_EVENT_t *&Event742, char *&buffer, uint32_t& allocatedSize);
void SaveCorrectionTable(char *outputFileName, DataCorrection_t* tb, QTextStream& textStream); 

bool WriteDataHeader(FILE* file, DataHeader* header);
bool WriteDataHeaderCompressed(gzFile file, DataHeader* header);

bool AppendEvent(FILE* file, EventVx* ev);
bool AppendEventCompressed(gzFile file, EventVx* ev);
bool AppendEvent742(FILE* file, EventVx* ev);
bool AppendEvent742Compressed(gzFile file, EventVx* ev);
bool UpdateEventHeader(FILE* file, DataHeader* header);


