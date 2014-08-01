#pragma once

#include <qwt_plot.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

class QwtPlotCurve;
class QwtSymbol;

class SortedPairPlot : public QwtPlot
{
 
public:
    SortedPairPlot( QWidget *parent = NULL );

    void setSymbol( QwtSymbol * );
    void setSamples( const QVector<double> &samplesX,  const QVector<double> &samplesY);

private:
    QwtPlotCurve *d_curve;
	QwtPlotCurve *d_curve_fit;
	QwtPlotCurve *d_curve_proportional;
};