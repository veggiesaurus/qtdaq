#pragma once

#include <QDialog>
#include "ui_dialogLinearCut.h"

class LinearCutDialog : public QDialog
{
	Q_OBJECT
public:
	LinearCutDialog ( QWidget * parent = 0, Qt::WindowFlags f = 0 );
private:
	void autoUpdateName();
public:
	Ui::DialogLinearCut ui;
private:
	bool autoName;

public slots:
	void onNameChanged();
	void onLinearCutChanged();
	void onChannelChanged();
	void onParameterChanged();
};

