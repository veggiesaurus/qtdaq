#pragma once

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

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