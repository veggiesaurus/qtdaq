#pragma once

#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_renderer.h>

#include <QSettings>

#include "globals.h"

class CalibratedPlot: public QwtPlot
{
	Q_OBJECT
public:
    CalibratedPlot(QWidget * = NULL);
	void setXRangeParameter(double minQL, double maxQL);
	void setXRangeEnergy(double minE, double maxE);
	void setYRange(double minY, double maxY);
	void setCalibrationValues(EnergyCalibration s_energyCalibration);
	void exportPlot(QString filename);
	virtual void setProjectorMode(bool projectorMode);
	QwtPicker* picker;

protected:
	QwtPlotGrid *grid;
	double minQL, maxQL;
	EnergyCalibration energyCalibration;
};