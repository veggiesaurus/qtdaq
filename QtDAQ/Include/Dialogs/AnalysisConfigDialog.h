#pragma once


#include <QDialog>
#include <QCheckBox>
#include "ui_dialogAnalysisConfig.h"
#include "AnalysisConfig.h"
#include "DRS4/DRS.h"

class AnalysisConfigDialog : public QDialog
{
	Q_OBJECT
public:
	AnalysisConfigDialog (AnalysisConfig* s_config, QWidget * parent = 0, Qt::WindowFlags f = 0 );
	void updateConfig();
private:
	void setUIFromConfig(AnalysisConfig* s_config);
	
	AnalysisConfig* config;
private slots:
	void updateUI();
signals:
	void analysisConfigChanged(AnalysisConfig*);
private:
	Ui::DialogAnalysisConfig ui;
};
