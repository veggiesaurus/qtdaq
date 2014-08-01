#pragma once
#include "DRS4Acquisition.h"
#include "ui_windowSortedPairPlot.h"
#include "ui_dialogSortedPairPlotConfig.h"
#include "Windows/PlotWindow.h"
#include "Plots/SortedPairPlot.h"

class SortedPairPlotWindow : public PlotWindow
{
	Q_OBJECT
public:
	SortedPairPlotWindow (QWidget * parent = 0, int s_chPrimary=0, int s_chSecondary=1, HistogramParameter s_parameter=LONG_INTEGRAL, int s_refreshDelay=100, bool s_autoscale=false);
	void onNewEventStatistics(QVector<EventStatistics*>* events);
	~SortedPairPlotWindow ();

public slots:
	void onOptionsClicked();
	void clearValues();
	void timerUpdate();
	void onSaveDataClicked();
signals:
	void channelChanged(int, int);
private:
	void updateTitleAndAxis();

private:
	Ui::WindowSortedPairPlot ui;
	Ui::DialogSortedPairPlotConfig uiDialogSortedPairPlotConfig;
	
	QVector<double> xVals, yVals;
public:
	int chSecondary;
	HistogramParameter parameter;
	int refreshDelay;
	bool autoscale;	
};
