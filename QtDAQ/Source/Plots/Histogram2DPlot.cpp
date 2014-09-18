#include "Histogram2DPlot.h"

ColorMap::ColorMap(): QwtLinearColorMap(Qt::white, Qt::darkRed)
{
	//addColorStop(0.0, Qt::white);
	addColorStop(0.0001, Qt::darkBlue);
    addColorStop(0.4, Qt::cyan);
    addColorStop(0.6, Qt::yellow);
    addColorStop(0.8, Qt::red);
}

LogarithmicColorMap::LogarithmicColorMap(): QwtLinearColorMap(Qt::white, Qt::darkRed)        
{
	//addColorStop(0.0, Qt::white);
	addColorStop(0.0001, Qt::darkBlue);
	addColorStop(0.4, Qt::cyan);
	addColorStop(0.6, Qt::yellow);
	addColorStop(0.8, Qt::red);
}

QRgb LogarithmicColorMap::rgb(const QwtInterval &interval, double value) const
{
    return QwtLinearColorMap::rgb(QwtInterval(log10(interval.minValue()),
                                                log10(interval.maxValue())),
                                    log10(value));
}

RasterData::RasterData(QVector<double> &values, int numColumns, double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{	
	setValueMatrix(values, numColumns);

	setInterval( Qt::XAxis, 
		QwtInterval( xMin, xMax, QwtInterval::ExcludeMaximum ) );
	setInterval( Qt::YAxis, 
		QwtInterval( yMin, yMax, QwtInterval::ExcludeMaximum ) );
	setInterval( Qt::ZAxis, QwtInterval(zMin, zMax) );
}

Histogram2DPlot::Histogram2DPlot(QWidget *parent):
	CalibratedPlot( parent )
{
    setAxisTitle(xBottom, "Long Gate Integral" );
    setAxisScale(xBottom, 0.0, 2000.0);

    setAxisTitle(yLeft, "PSD Factor");
    setAxisScale(yLeft, 0.0, 100.0);
	d_spectrogram = new QwtPlotSpectrogram();
    d_spectrogram->setRenderThreadCount(0); // use system specific thread count	
	if (logScaleEnabled)
		d_spectrogram->setColorMap(new LogarithmicColorMap());
	else 
		d_spectrogram->setColorMap(new ColorMap());
	d_spectrogram->attach(this);   

    // A color bar on the right axis
    rightAxis = axisWidget(QwtPlot::yRight);
    rightAxis->setColorBarEnabled(true);
    rightAxis->setColorBarWidth(40);
   
    enableAxis(QwtPlot::yRight);

	setCanvasBackground(QBrush(Qt::white));
	//picker
	plotPicker = new QwtPlotPicker(this->xBottom, this->yLeft, QwtPicker::PolygonRubberBand, QwtPicker::AlwaysOn, this->canvas());
	QPen polygonPickerPen(Qt::darkRed, 2, Qt::DashLine);
	plotPicker->setRubberBandPen(polygonPickerPen);	
	plotPicker->setEnabled(false);
	pickerPolygonMachine = new QwtPickerPolygonMachine ();
	pickerPointMachine = new QwtPickerClickPointMachine ();
	plotPicker->setStateMachine(pickerPolygonMachine);
	
    connect(plotPicker, SIGNAL(selected(const QVector< QPointF >&)), this, SLOT(onSelected(const QVector< QPointF >&)));

	selectedPolygon=new QwtPlotShapeItem("Cut");
	selectedPolygon->setVisible(false);	
	QBrush selectedPolygonCurveBrush(QColor(200,100,100,100));
	selectedPolygon->setBrush(selectedPolygonCurveBrush);
	QPen selectedPolygonCurvePen(Qt::white, 2, Qt::DashLine);
	selectedPolygon->setPen(selectedPolygonCurvePen);
	selectedPolygon->setRenderHint(QwtPlotItem::RenderAntialiased);
	selectedPolygon->attach(this);
	//event filters
	canvas()->installEventFilter(this);

	//markers for Sept10 TODO: remove
	QSettings settings;
	double neutronsX = settings.value("experimental/labels/neutronsX", QVariant(4000.0)).toDouble();
	double neutronsY = settings.value("experimental/labels/neutronsY", QVariant(50.0)).toDouble();
	double gammaX = settings.value("experimental/labels/gammaX", QVariant(4000.0)).toDouble();
	double gammaY = settings.value("experimental/labels/gammaY", QVariant(80.0)).toDouble();
	int fontSize = settings.value("experimental/labels/fontSize", QVariant(32)).toInt();
	//write settings back in case they weren't there
	settings.setValue("experimental/labels/neutronsX", QVariant(neutronsX));
	settings.setValue("experimental/labels/neutronsY", QVariant(neutronsY));
	settings.setValue("experimental/labels/gammaX", QVariant(gammaX));
	settings.setValue("experimental/labels/gammaY", QVariant(gammaY));
	settings.setValue("experimental/labels/fontSize", QVariant(fontSize));


	QFont markerFont;
	markerFont.setPixelSize(fontSize);
	
	QwtText neutronMarkerLabel;
	neutronMarker = new QwtPlotMarker();

	neutronMarkerLabel.setText("Neutrons");
	neutronMarkerLabel.setFont(markerFont);
	neutronMarker->setLabel(neutronMarkerLabel);
	neutronMarker->attach(this);
	neutronMarker->setVisible(true);
	neutronMarker->setXValue(neutronsX);
	neutronMarker->setYValue(neutronsY);

	

	QwtText gammaMarkerLabel;
	gammaMarker = new QwtPlotMarker();

	gammaMarkerLabel.setText("Gamma-rays");
	gammaMarkerLabel.setFont(markerFont);
	gammaMarker->setLabel(gammaMarkerLabel);
	gammaMarker->attach(this);
	gammaMarker->setVisible(true);
	gammaMarker->setXValue(gammaX);
	gammaMarker->setYValue(gammaY);

	//end Sept10
}

Histogram2DPlot::~Histogram2DPlot()
{
	SAFE_DELETE(d_spectrogram);
	SAFE_DELETE(plotPicker);
	SAFE_DELETE(selectedPolygon);
	SAFE_DELETE(pickerPointMachine);
}


void Histogram2DPlot::onSelected (const QVector< QPointF > &pa)
{	
	if (pa.size()>2)
	{		
		plotPicker->setEnabled(false);
		QVector<QPointF> appendedPointArray=pa;
		appendedPointArray.push_back(pa[0]);		
		selectedPolygon->setPolygon(appendedPointArray);
		selectedPolygon->setVisible(true);
		emit pointSelectionComplete(pa);
	}
}

void Histogram2DPlot::setSelectedPolygon(const QVector< QPointF > &pa)
{
	if (pa.size()>2)
	{		
		QVector<QPointF> appendedPointArray=pa;
		appendedPointArray.push_back(pa[0]);		
		selectedPolygon->setPolygon(appendedPointArray);
		selectedPolygon->setVisible(true);
	}
	else
		selectedPolygon->setVisible(false);
}

void Histogram2DPlot::enablePolygonPicker(bool enabled)
{	
	plotPicker->setEnabled(enabled);
}

void Histogram2DPlot::displayCutPolygon(bool display)
{
	selectedPolygon->setVisible(display);
}

void Histogram2DPlot::setRasterData(QVector<double> &values, int numColumns, double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
		
    d_spectrogram->setData(new RasterData(values, numColumns, xMin, xMax, yMin, yMax, fmax(zMin, 1.0), fmax(zMax, 1.0)));	
	setAxisScale(xBottom, xMin, xMax);
    setAxisScale(yLeft, yMin, yMax);
	const QwtInterval zInterval = d_spectrogram->data()->interval( Qt::ZAxis );
	if (logScaleEnabled)
		rightAxis->setColorMap(zInterval, new LogarithmicColorMap());
	else 
		rightAxis->setColorMap(zInterval, new ColorMap());	
    setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );	
}

void Histogram2DPlot::setResampleMode(int mode)
{
    RasterData *data = (RasterData *)d_spectrogram->data();
    data->setResampleMode( (QwtMatrixRasterData::ResampleMode) mode);
    replot();
}

void Histogram2DPlot::setLogScale(bool enabled)
{
	if (enabled!=logScaleEnabled)
	{
		logScaleEnabled=enabled;
		if (logScaleEnabled)
			d_spectrogram->setColorMap(new LogarithmicColorMap());
		else 
			d_spectrogram->setColorMap(new ColorMap());
	}
}