#include <qwt_series_data.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_grid.h>
#include "globals.h"
#include "Plots/HistogramPlot.h"



Histogram::Histogram(const QString &title, const QColor &symbolColor):
    QwtPlotHistogram(title)
{
    setStyle(QwtPlotHistogram::Columns);
    setColor(symbolColor);	
}

void Histogram::setColor(const QColor &symbolColor)
{
    QColor color = symbolColor;
    color.setAlpha(180);

    setPen(QPen(Qt::black));
    setBrush(QBrush(color));

    QwtColumnSymbol *symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    symbol->setFrameStyle(QwtColumnSymbol::Raised);
    symbol->setLineWidth(2);
    symbol->setPalette(QPalette(color));
    setSymbol(symbol);
}

void Histogram::setValues(uint numValues, const double *values, double minValue, double maxValue)
{
	double intervalValue=(maxValue-minValue)/numValues;

    QVector<QwtIntervalSample> samples(numValues);
    for ( uint i = 0; i < numValues; i++ )
    {
        QwtInterval interval(minValue+i*intervalValue, minValue+(i+1)*intervalValue);
        interval.setBorderFlags(QwtInterval::ExcludeMaximum);
        
        samples[i] = QwtIntervalSample(values[i], interval);
    }

    setData(new QwtIntervalSeriesData(samples));	
}

HistogramPlot::HistogramPlot(QWidget *parent):
    CalibratedPlot( parent )
{	
	connect(picker, SIGNAL(activated(bool)), SLOT(pickerActivated()));
	
	logScaleEngine=new QwtLogScaleEngine();
	linearScaleEngine=new QwtLinearScaleEngine();
	
	histogram=new Histogram(NULL, Qt::red);	
	histogram->attach(this);
	histogram->setStyle(QwtPlotHistogram::Outline);

	fittedCurve=new QwtPlotCurve("Fitted Function");
	fittedCurve->attach(this);
	fittedCurve->setVisible(false);
}

HistogramPlot::~HistogramPlot()
{
	SAFE_DELETE(logScaleEngine);
	SAFE_DELETE(linearScaleEngine);
	SAFE_DELETE(histogram);
	SAFE_DELETE(fittedCurve);
}

void HistogramPlot::setLogScale(bool log)
{
	if (log)
	{
		setAxisScaleEngine(yLeft, new QwtLogScaleEngine());
		setAxisAutoScale(yLeft, false);
		histogram->setBaseline(LOG_MIN);	
		replot();
	}
	else
	{
		setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
		setAxisAutoScale(QwtPlot::yLeft, true);
		replot();
	}
}

void HistogramPlot::setFittedValues(uint numValues, const double* xVals, const double* yVals)
{
	fittedCurve->setSamples(xVals, yVals, numValues);
	fittedCurve->setVisible(true);
}

void HistogramPlot::pickerActivated()
{

}