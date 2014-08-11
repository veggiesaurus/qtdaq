#pragma once

#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_histogram.h>
#include <qwt_series_data.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_engine.h>
#include "Plots/CalibratedPlot.h"
#include "globals.h"

class Histogram: public QwtPlotHistogram
{
public:
    Histogram(const QString &, const QColor &);
    void setColor(const QColor &);
    void setValues(uint numValues, const double *, double minValue, double maxValue);
	
};


class HistogramPlot: public CalibratedPlot
{	 
	Q_OBJECT
public:
    HistogramPlot(QWidget * = NULL);
	~HistogramPlot();
	void setLogScale(bool log);
	void setValues(uint numValues, const double* values, double minValue, double maxValue){histogram->setValues(numValues, values, minValue, maxValue);}
	void setFittedValues(uint numValues, const double* xVals, const double* yVals);
	QwtSeriesData<QwtIntervalSample>* getValues(){return histogram->data();}
public slots:
	void pickerActivated();
private:
	Histogram* histogram;
	QwtLogScaleEngine* logScaleEngine;
	QwtLinearScaleEngine* linearScaleEngine;
	QwtPlotCurve* fittedCurve;
};
