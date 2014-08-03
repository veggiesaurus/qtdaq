#pragma once

#include <QPoint>
//#include <qgl.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_grid.h>
#include <qwt_picker_machine.h>
#include "ui_windowFoMPlot.h"
#include "ui_dialogHistogram2DConfig.h"
#include "Windows/PlotWindow.h"
#include "Plots/CalibratedPlot.h"
#include "curvefit/fitting.h"

class FoMWindow : public PlotWindow
{
	Q_OBJECT


public:
	FoMWindow (QWidget * parent=0, HistogramParameter s_parameterX=LONG_INTEGRAL, HistogramParameter s_parameterY=PSD_FACTOR, int s_chPrimary=0, int s_parameterMinX=0, int s_parameterMaxX=100000, int s_parameterMinY=0, int s_parameterMaxY=100, int s_numBinsX=64, int s_numBinsY=256);
	~FoMWindow ();
	

public slots:
	void onOptionsClicked();
	void onRefreshClicked();
	void onNewCalibration(int channel, EnergyCalibration calibration);
	void onNewEventStatistics(QVector<EventStatistics*>* events);
	void clearValues();
	void timerUpdate();
	void onSaveDataClicked();
signals:
	void channelChanged(int);

private:
	void updateTitleAndAxis();
	//called after serialization
	void updateSettings();
	double getFigureOfMerit(int indexX);

private:
	Ui::WindowFoMPlot ui;
	Ui::DialogHistogram2DConfig uiDialogHistogramConfig;
	QwtPlotCurve* fomCurve;
	QwtPlotMarker *effectiveSepMarker;
	
	HistogramParameter parameterX, parameterY;
	int numBinsX,numBinsY;
	double parameterMinX, parameterMaxX;
	double parameterMinY, parameterMaxY;
	double parameterIntervalX;
	double parameterIntervalY;	
	QVector<double>values;
	double *xVals;
	double* yVals;
	
	double* tempVals;
	double *A1, *A2;
	double *x_c1, *x_c2;
	double *sigma1, *sigma2;

private:
	friend QDataStream &operator<<(QDataStream &out, const FoMWindow &obj);
	friend QDataStream &operator>>(QDataStream &in, FoMWindow &obj);
};