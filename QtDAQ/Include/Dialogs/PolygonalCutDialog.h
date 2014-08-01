#pragma once

#include <QDialog>
#include "ui_dialogPolygonalCut.h"
#include "globals.h"
#include "Cuts.h"


class PolygonalCutDialog : public QDialog
{
	Q_OBJECT
public:
	PolygonalCutDialog (int channel, HistogramParameter parameterX, HistogramParameter parameterY, QWidget * parent = 0, Qt::WindowFlags f = 0);
private:
	void autoUpdateName();
public:
	Ui::DialogPolygonalCut ui;
private:
	bool autoName;

public slots:
	void onNameChanged();
	void onChannelChanged();
	void onRedrawClicked();
};
