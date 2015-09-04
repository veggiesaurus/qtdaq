#pragma once

#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_matrix_raster_data.h>
#include <qwt_color_map.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_shapeitem.h>
#include "Plots/CalibratedPlot.h"


class RasterData: public QwtMatrixRasterData
{
public:
    RasterData(QVector<double> &values, int numColumns, double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);    
};

class ColorMap: public QwtLinearColorMap
{
public:
    ColorMap();
};

class LogarithmicColorMap : public QwtLinearColorMap
{
public:
	LogarithmicColorMap();
    QRgb rgb(const QwtInterval &interval, double value) const;
};

class Histogram2DPlot: public CalibratedPlot
{
	Q_OBJECT
public:
	Histogram2DPlot(QWidget * = NULL);
	~Histogram2DPlot();
	void setRasterData(QVector<double> &values, int numColumns, double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
	void setResampleMode(int mode);
	void setLogScale(bool enabled);
	void enablePolygonPicker(bool enabled);
	void displayCutPolygon(bool display);	
public slots:
	void onSelected(const QVector< QPointF >& pa);
	void setSelectedPolygon(const QVector< QPointF > &pa);
signals:
	void pointSelectionComplete(const QVector< QPointF >&pa);
private:
	QwtPlotSpectrogram *d_spectrogram;	
	QwtScaleWidget *rightAxis;
	QwtPicker* plotPicker;
	QwtPickerPolygonMachine* pickerPolygonMachine;
	QwtPickerClickPointMachine* pickerPointMachine;
	QwtPlotShapeItem* selectedPolygon;
	bool logScaleEnabled = false;

	QwtPlotMarker* neutronMarker=nullptr;
	QwtPlotMarker* gammaMarker=nullptr;
};
