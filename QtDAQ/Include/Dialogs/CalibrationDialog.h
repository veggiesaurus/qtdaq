#pragma once

#include <QDialog>
#include "ui_dialogCalibration.h"


class CailbrationDialog : public QDialog
{
	Q_OBJECT
public:
	CailbrationDialog ( QWidget * parent = 0, Qt::WindowFlags f = 0 );
private:
	void calculateCalibrationValues();

public:
	Ui::DialogCalibration ui;
private:
	double scale, offset;
	bool validData;
public slots:
	void onDataEntryChanged(int xIndex,int yIndex);
};

