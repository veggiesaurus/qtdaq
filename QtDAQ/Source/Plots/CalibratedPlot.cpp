#include "Plots/CalibratedPlot.h"

CalibratedPlot::CalibratedPlot(QWidget *parent):
    QwtPlot( parent )
{
    setAutoReplot(false);
	picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
        QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, 
        canvas());
    picker->setStateMachine(new QwtPickerClickPointMachine());
    picker->setRubberBandPen(QColor(Qt::darkBlue));
    picker->setRubberBand(QwtPicker::CrossRubberBand);
    picker->setTrackerPen(QColor(Qt::black));
    
	setAxisTitle(xBottom, "Parameter" );
    setAxisTitle(xTop, "Energy (MeVee)");
    setAxisTitle(yLeft, "Count");

	// grid 
    grid = new QwtPlotGrid();
    grid->enableXMin(true);
	grid->enableY(false);
	grid->enableX(false);
	grid->setXAxis(xTop);
	grid->setPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);
	setCanvasBackground(QBrush(Qt::white));
	energyCalibration.calibrated=false;
}

void CalibratedPlot::setCalibrationValues(EnergyCalibration s_energyCalibration)
{	
	energyCalibration=s_energyCalibration;
	setXRangeParameter(minQL, maxQL);
	enableAxis(xTop, energyCalibration.calibrated);
	grid->enableX(energyCalibration.calibrated);
	grid->setXAxis(xTop);
	
}

void CalibratedPlot::setXRangeParameter(double s_minQL, double s_maxQL)
{
	minQL=s_minQL;
	maxQL=s_maxQL;
	setAxisScale(QwtPlot::xBottom, minQL, maxQL);
	if (energyCalibration.calibrated)
	{
		double minE=(minQL-energyCalibration.offset)/energyCalibration.scale;
		double maxE=(maxQL-energyCalibration.offset)/energyCalibration.scale;
		setAxisScale(QwtPlot::xTop, minE, maxE);
	}
	replot();
}

void CalibratedPlot::setXRangeEnergy(double minE, double maxE)
{
	setAxisScale(QwtPlot::xTop, minE, maxE);
	if (energyCalibration.calibrated)
	{
		double minQL=energyCalibration.offset+minE*energyCalibration.scale;
		double maxQL=energyCalibration.offset+maxE*energyCalibration.scale;
		setAxisScale(QwtPlot::xBottom, minQL, maxQL);
	}
	replot();
}

void CalibratedPlot::setYRange(double minY, double maxY)
{
	setAxisScale(QwtPlot::yLeft, minY, maxY);
	replot();
}


void CalibratedPlot::exportPlot(QString filename)
{ 
    if ( !filename.isEmpty() )
    {
        QwtPlotRenderer renderer;		
		//TODO: replace KeepFrames
		//renderer.setLayoutFlag(QwtPlotRenderer::KeepFrames, true);
        renderer.renderDocument(this, filename, QSizeF(320, 180), 85);
    }
}