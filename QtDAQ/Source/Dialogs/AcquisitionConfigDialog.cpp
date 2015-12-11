#include <QMessageBox>
#include "globals.h"
#include "Dialogs/AcquisitionConfigDialog.h"



AcquisitionConfigDialog::AcquisitionConfigDialog(DRSAcquisitionConfig* s_config, QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	config=s_config;

#pragma region ui array instantiation
	//Enable Checkboxes 
	checkBoxEnabled[0]=ui.checkBoxCh0Enabled;
	checkBoxEnabled[1]=ui.checkBoxCh1Enabled;
	checkBoxEnabled[2]=ui.checkBoxCh2Enabled;
	checkBoxEnabled[3]=ui.checkBoxCh3Enabled;
	
	//Trigger Enable Checkboxes 
	checkBoxTriggerEnabled[0]=ui.checkBoxCh0TriggerEnabled;
	checkBoxTriggerEnabled[1]=ui.checkBoxCh1TriggerEnabled;
	checkBoxTriggerEnabled[2]=ui.checkBoxCh2TriggerEnabled;
	checkBoxTriggerEnabled[3]=ui.checkBoxCh3TriggerEnabled;	

#pragma endregion (converting from names in Qt Designer UI file to an array for ease of use)

#pragma region connections (connection UI elements to slots)
	for (int i=0;i<NUM_DIGITIZER_CHANNELS_DRS;i++)
	{
		connect(checkBoxEnabled[i], SIGNAL(stateChanged(int)), this, SLOT(onChannelCheckBoxChanged()));
		connect(checkBoxTriggerEnabled[i], SIGNAL(stateChanged(int)), this, SLOT(onChannelTriggerCheckBoxChanged()));
	}
	
	connect(ui.comboBoxSelfTriggering, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxSelfTriggeringChanged()));

#pragma endregion

	setUIFromConfig(config);

}

void AcquisitionConfigDialog::setDRS(DRS* s_drs)
{
	drs=s_drs;
	char strError[255];
	memset(strError, 0, 255);

#pragma region DRS connection

	if (drs==NULL)
		drs=new DRS();
	drs->GetError(strError, 255);
	int numBoards=drs->GetNumberOfBoards();
	while (!numBoards)
	{
		SAFE_DELETE(drs);
		int retryResponse=QMessageBox::warning(this, "DRS Board Error", "No DRS board was found. Press retry to try again, or cancel to ignore.",
                                QMessageBox::Retry | QMessageBox::Cancel,
                               QMessageBox::Retry);
		if (retryResponse==QMessageBox::Retry)
		{
			drs=new DRS();
			numBoards=drs->GetNumberOfBoards();
			if (drs && numBoards)
			{
				auto boardType = drs->GetBoard(0)->GetBoardType();
				if (!boardType)
					numBoards = 0;
			}
		}
		else
		{
			drs=NULL;
			board=NULL;
			break;
		}
	}
	//get board zero
	if (drs)
		board=drs->GetBoard(0);
	emit drsObjectChanged(drs, board);

	if (board)
	{
		QString info;
		char drsInfo[255];
		sprintf(drsInfo, "DRS type:             DRS%d\n", board->GetDRSType());
		info+=drsInfo;
        sprintf(drsInfo, "Board type:           %d\n", board->GetBoardType());
		info+=drsInfo;
        sprintf(drsInfo, "Serial number:        %04d\n", board->GetBoardSerialNumber());
		info+=drsInfo;
        sprintf(drsInfo, "Firmware revision:    %d\n", board->GetFirmwareVersion());
		info+=drsInfo;
        sprintf(drsInfo, "Temperature:          %1.1lf C\n", board->GetTemperature());
		info+=drsInfo;
        if (board->GetDRSType() == 4) 
		{
            sprintf(drsInfo, "Input range:          %1.2lgV...%1.2lgV\n", 
                    board->GetInputRange()-0.5, board->GetInputRange()+0.5);
			info+=drsInfo;
            sprintf(drsInfo, "Calibrated range:     %1.2lgV...%1.2lgV\n", board->GetCalibratedInputRange()-0.5,
                board->GetCalibratedInputRange()+0.5);
			info+=drsInfo;
            sprintf(drsInfo, "Calibrated frequency: %1.3lf GHz\n", board->GetCalibratedFrequency());
			info+=drsInfo;
		}
		ui.textBrowserInfo->setText(info);
	}

#pragma endregion
}

/////////////////////////////////////////////////
// initialises the UI based on a config object
/////////////////////////////////////////////////
void AcquisitionConfigDialog::setUIFromConfig(DRSAcquisitionConfig* s_config)
{
#pragma region global (global digitizer settings)
	//////////////////////////////
	//global digitizer settings 
	//////////////////////////////	
	ui.comboBoxCorrectionLevel->setCurrentIndex(s_config->correctionLevel);
    ui.spinBoxVoltageOffset->setValue(qMin(500, qMax(-500, (int)s_config->voltageOffset)));

	//samples per event, set to 1024
	switch (s_config->samplesPerEvent)
	{	
	default:
		ui.comboBoxSamplesPerEvent->setCurrentIndex(0);
		break;
	}

	//sampling rate
	ui.comboBoxSampleRate->setCurrentText(QString::number(config->sampleRateMSPS));
	
#pragma endregion

#pragma region trigger (global trigger settings)
	//////////////////////////////
	//global trigger settings   
	//////////////////////////////
	ui.comboBoxExternalTrigger->setCurrentIndex(s_config->externalTriggeringEnabled);
	ui.comboBoxSelfTriggering->setCurrentIndex(s_config->selfTriggeringEnabled);
	ui.comboBoxTriggerEdge->setCurrentIndex(s_config->triggerPolarityIsNegative);
    ui.spinBoxTriggerDelay->setValue(qMin(100, qMax(0, (int)s_config->postTriggerDelay)));
    ui.spinBoxTriggerThreshold->setValue(qMin(500, qMax(-500, (int)s_config->triggerThreshold)));
	ui.comboBoxTriggerEdge->setCurrentIndex(s_config->triggerPolarityIsNegative);
#pragma endregion

#pragma region channels
	//////////////////////////////
	// channels
	//////////////////////////////
	for (int i=0;i<NUM_DIGITIZER_CHANNELS_DRS;i++)
	{
		checkBoxEnabled[i]->setChecked(config->channelEnabled[i]);
		//conditions for being allowed to select trigger: channel enabled and self triggering enabled
		checkBoxTriggerEnabled[i]->setEnabled(checkBoxEnabled[i]->checkState()  && ui.comboBoxSelfTriggering->currentIndex());		
		checkBoxTriggerEnabled[i]->setChecked(config->channelSelfTriggerEnabled[i]);

	}
#pragma endregion

}

void AcquisitionConfigDialog::updateUI()
{		
	//update channels
	for (int i=0;i<NUM_DIGITIZER_CHANNELS_DRS;i++)
	{		
		//conditions for being allowed to select trigger: channel enabled and self triggering enabled
		checkBoxTriggerEnabled[i]->setEnabled(checkBoxEnabled[i]->checkState() && ui.comboBoxSelfTriggering->currentIndex());		
	}
}

/////////////////////////////////////////////////
// updates the config based on UI
/////////////////////////////////////////////////
void AcquisitionConfigDialog::updateConfig()
{
#pragma region global (global digitizer settings)
	//////////////////////////////
	//global digitizer settings 
	//////////////////////////////
		
	config->correctionLevel=ui.comboBoxCorrectionLevel->currentIndex();	
	config->voltageOffset=ui.spinBoxVoltageOffset->value();
	//samples per event
	int samplesIndex=ui.comboBoxSamplesPerEvent->currentIndex();
	switch (samplesIndex)
	{
		default:
			config->samplesPerEvent=1024;
		break;
	}

	//sample rate
	int sampleRate=ui.comboBoxSampleRate->currentText().toInt();
	//range is between 698MS and 5.12GS
    sampleRate=qMin(sampleRate, 5120);
    sampleRate=qMax(sampleRate, 698);
	config->sampleRateMSPS=sampleRate;
	config->requiresReconfig = true;
#pragma endregion

#pragma region trigger (global trigger settings)
	//////////////////////////////
	//global trigger settings   
	//////////////////////////////
	config->selfTriggeringEnabled=ui.comboBoxSelfTriggering->currentIndex();
	config->triggerPolarityIsNegative=ui.comboBoxTriggerEdge->currentIndex();
	config->postTriggerDelay=ui.spinBoxTriggerDelay->value();
	config->triggerThreshold=ui.spinBoxTriggerThreshold->value();
#pragma endregion

#pragma region channels
	//////////////////////////////
	// channels
	//////////////////////////////

	for (int i=0;i<NUM_DIGITIZER_CHANNELS_DRS;i++)
	{				
		config->channelEnabled[i]=checkBoxEnabled[i]->isChecked();
		config->channelSelfTriggerEnabled[i]=checkBoxTriggerEnabled[i] && checkBoxEnabled[i]->isChecked();
	}
#pragma endregion
}

#pragma region slots (slots for ui interactions)

void AcquisitionConfigDialog::onChannelCheckBoxChanged()
{		
	updateUI();
}

void AcquisitionConfigDialog::onChannelTriggerCheckBoxChanged()
{
	updateUI();
}

void AcquisitionConfigDialog::onComboBoxSelfTriggeringChanged()
{
	updateUI();
}

void AcquisitionConfigDialog::onVoltageOffsetChanged()
{
	updateUI();
}

void AcquisitionConfigDialog::onTriggerThresholdChanged()
{
	updateUI();
}

#pragma endregion
