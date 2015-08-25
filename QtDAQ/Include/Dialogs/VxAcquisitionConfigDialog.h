#pragma once

#include <QDialog>
#include <QMessageBox>
#include <QDebug>
#include "ui_dialogVxAcquisitionConfig.h"
#include "globals.h"
#include "VxHighlighter.h"

class VxAcquisitionConfigDialog : public QDialog
{
	Q_OBJECT
public:
	VxAcquisitionConfigDialog(QString s_config, QWidget * parent = 0, Qt::WindowFlags f = 0);

public:
	Ui::DialogVxAcquisitionConfig ui;

public slots:
	void configChanged();

private:
	QString config;
	VxHighlighter* highlighter;

};
