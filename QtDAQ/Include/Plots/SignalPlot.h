#pragma once


#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_picker.h>
#include "globals.h"

class SignalPlot: public QwtPlot
{
	Q_OBJECT
public:
    SignalPlot(QWidget * = NULL);
	void setBaseline(double baseline);
	void setDisplayAverage(bool s_displayAverage);
	void setTrigger(float trigger);
	void setGates(int start, int shortGateEnd, int longGateEnd, int tofStartPulse, int tofEndPulse);
	void setData(float* t, float* V, float* DV, int s_numSamples, float s_sampleTime, float s_offsetTime);
	void clearAverages();
	void setAlignSignals(bool);
	void showToFGates(bool);
	void showCIGates(bool);
	void setProjectorMode(bool);
public:
	//temp arrays for storing the signal;
	double* tempT=nullptr, *tempV=nullptr, *tempDV=nullptr;
	//averaging and shifting
	int numAveragedEvents=0;
	double* vAverage=nullptr;
	double* vShifted=nullptr;
	double initBaseline=0;
	int initOffset=0;

	double sampleTime;
	int numSamples=0;
	bool displayAverage=false;

private:
	QwtPlotMarker *baseline;
	QwtPlotMarker *triggerLine;

	QwtPlotMarker *peakPosition;
	QwtPlotMarker *halfPeakPosition;
	
	QwtPlotMarker *startPosition;
	QwtPlotMarker *shortGateEndPosition;
	QwtPlotMarker *longGateEndPosition;
	QwtPlotMarker *tofStartPosition;
	QwtPlotMarker *tofEndPosition;
    
	QwtPlotCurve *cSignal;
	QwtPlotCurve *cDeltaSignal;

	bool alignSigs = false;
	bool tofGatesVisible = false;
	bool ciGatesVisible = true;

private:
	void shiftSample(float* sample, int numSamples, int shift);

};