#pragma once

#include <QDialog>
#include <QCheckBox>

#include "ui_dialogAcquisitionConfig.h"
#include "DRSAcquisitionConfig.h"
#include "DRS4/DRS.h"

class AcquisitionConfigDialog : public QDialog
{
	Q_OBJECT
public:
	AcquisitionConfigDialog(DRSAcquisitionConfig* s_config, QWidget * parent = 0, Qt::WindowFlags f = 0);
	void updateConfig();
	void setDRS(DRS* s_drs);
private:
	void setUIFromConfig(DRSAcquisitionConfig* s_config);
	void updateUI();
	
public:
	Ui::DialogAcquisitionConfig ui;
public slots:
	//channels
	void onChannelCheckBoxChanged();
	void onChannelTriggerCheckBoxChanged();
	
	void onComboBoxSelfTriggeringChanged();
	void onVoltageOffsetChanged();
	void onTriggerThresholdChanged();
signals:
	void drsObjectChanged(DRS* drs, DRSBoard* board);
private:
	DRSAcquisitionConfig* config;
	DRS* drs;
	DRSBoard* board;
	//UI arrays
	QCheckBox* checkBoxEnabled[NUM_DIGITIZER_CHANNELS_DRS];
	QCheckBox* checkBoxTriggerEnabled[NUM_DIGITIZER_CHANNELS_DRS];
};
