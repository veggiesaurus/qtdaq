#pragma once

#include "ui_windowHistogramPlot.h"
#include "ui_dialogHistogramConfig.h"
#include "Windows/PlotWindow.h"
#include "Dialogs/LinearCutDialog.h"


class HistogramWindow : public PlotWindow
{
	Q_OBJECT
public:
	HistogramWindow (QWidget * parent=0, HistogramParameter s_parameter=LONG_INTEGRAL, int s_chPrimary=0, double s_parameterMin=0, double s_parameterMax=100000, int s_numBins=512, bool s_delta=false, bool s_logScale=false);
	void setProjectorMode(bool s_projectorMode);

	~HistogramWindow ();


public slots:
	void onOptionsClicked();
	void onFitGaussianClicked();
	void onFitDoubleGaussianClicked();
	void onNewCalibration(int channel, EnergyCalibration calibration);
	void onNewEventStatistics(QVector<EventStatistics*>* events);
	void timerUpdate();

	void clearValues();	
	void onSaveDataClicked();
	void onSaveDataHEPROClicked();

	void onLogScaleToggled(bool logScaleEnabled);
	void onDeltaHistogramToggled(bool deltaEnabled);
	void onDuplicateClicked();
	void onAddLinearCutClicked();
signals:
	void channelChanged(int);
	void duplicate(HistogramWindow*);
	void linearCutCreated(LinearCut);
private:
	void updateTitleAndAxis();
	void recreateHistogram();
	//called after serialization
	void updateSettings();
private:
	Ui::WindowHistogramPlot ui;
	Ui::DialogHistogramConfig uiDialogHistogramConfig;

	HistogramParameter parameter;
	int numBins;
	double parameterMin, parameterMax;
	double parameterInterval;
	double* binVals;
	bool displayMeanAndFWHM, displayFoM, delta, logScale;
	bool autoFitGaussian, autoFitDoubleGaussian;
	double FWHM, mean, fom;

	//fitting variables
	double A1, A2;
	double x_c1, x_c2;
	double sigma1, sigma2;

private:
	friend QDataStream &operator<<(QDataStream &out, const HistogramWindow &obj);
	friend QDataStream &operator>>(QDataStream &in, HistogramWindow &obj);
};
