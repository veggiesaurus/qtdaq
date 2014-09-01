#include "Plots/signalplot.h"


SignalPlot::SignalPlot(QWidget *parent):
    QwtPlot( parent )
{
    setAutoReplot(false);

    // axes
    setAxisTitle(xBottom, "time (n s)" );

    setAxisTitle(yLeft, "voltage (mV)");
	//TODO: this is for deltas
	setAxisScale(yLeft, 0, 1200);

	//setAxisScaleDiv(yLeft, QwtScaleDiv()
	setCanvasBackground(Qt::white);
	//canvas()->setLineWidth( 1 );
    //canvas()->setFrameStyle( QFrame::Box | QFrame::Plain );

	 //baseline marker
	QPen baselinePen(Qt::darkRed,1);
    baseline=new QwtPlotMarker();
    baseline->setLabel(QString::fromLatin1("\t\tBaseline\t\t"));
	baseline->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    baseline->setLineStyle(QwtPlotMarker::HLine);
	baseline->setLinePen(baselinePen);	
    baseline->attach(this);
	baseline->setVisible(true);
	baseline->setYValue(512);

	triggerLine=new QwtPlotMarker();
    triggerLine->setLabel(QString::fromLatin1("\t\tTrigger Line\t\t"));
	triggerLine->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    triggerLine->setLineStyle(QwtPlotMarker::HLine);
	triggerLine->setLinePen(baselinePen);	
    triggerLine->attach(this);
	triggerLine->setVisible(false);

	 //peak marker
	QPen peakPositionPen(Qt::red,1);
    peakPosition=new QwtPlotMarker();
    peakPosition->setLabel(QString::fromLatin1("\t\tPeak\t\t"));
	peakPosition->setLabelOrientation(Qt::Vertical);
    peakPosition->setLabelAlignment(Qt::AlignRight|Qt::AlignBottom);
    peakPosition->setLineStyle(QwtPlotMarker::VLine);
	peakPosition->setLinePen(peakPositionPen);	
    peakPosition->attach(this);
	peakPosition->setVisible(false);

	halfPeakPosition=new QwtPlotMarker();
    halfPeakPosition->setLabel(QString::fromLatin1("\t\tHalf Peak\t\t"));
	halfPeakPosition->setLabelOrientation(Qt::Vertical);
    halfPeakPosition->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    halfPeakPosition->setLineStyle(QwtPlotMarker::VLine);
	halfPeakPosition->setLinePen(peakPositionPen);	
    halfPeakPosition->attach(this);
	halfPeakPosition->setVisible(false);

	QPen gatePositionPen(Qt::blue,1);
	startPosition=new QwtPlotMarker();
    startPosition->setLabel(QString::fromLatin1("\t\tStart Gate\t\t"));
	startPosition->setLabelOrientation(Qt::Vertical);
    startPosition->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    startPosition->setLineStyle(QwtPlotMarker::VLine);
	startPosition->setLinePen(gatePositionPen);	
    startPosition->attach(this);
	startPosition->setVisible(false);	
	
	shortGateEndPosition=new QwtPlotMarker();
    shortGateEndPosition->setLabel(QString::fromLatin1("\t\tShort Gate\t\t"));
	shortGateEndPosition->setLabelOrientation(Qt::Vertical);
    shortGateEndPosition->setLabelAlignment(Qt::AlignRight|Qt::AlignBottom);
    shortGateEndPosition->setLineStyle(QwtPlotMarker::VLine);
	shortGateEndPosition->setLinePen(gatePositionPen);	
    shortGateEndPosition->attach(this);
	shortGateEndPosition->setVisible(false);
	
	longGateEndPosition=new QwtPlotMarker();
    longGateEndPosition->setLabel(QString::fromLatin1("\t\tLong Gate\t\t"));
	longGateEndPosition->setLabelOrientation(Qt::Vertical);
    longGateEndPosition->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    longGateEndPosition->setLineStyle(QwtPlotMarker::VLine);
	longGateEndPosition->setLinePen(gatePositionPen);	
    longGateEndPosition->attach(this);
	longGateEndPosition->setVisible(false);

	QPen tofPositionPen(Qt::darkGreen,1);
	tofStartPosition=new QwtPlotMarker();
    tofStartPosition->setLabel(QString::fromLatin1("\t\tTOF Start\t\t"));
	tofStartPosition->setLabelOrientation(Qt::Vertical);
    tofStartPosition->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    tofStartPosition->setLineStyle(QwtPlotMarker::VLine);
	tofStartPosition->setLinePen(tofPositionPen);	
    tofStartPosition->attach(this);
	tofStartPosition->setVisible(false);	
	
	tofEndPosition=new QwtPlotMarker();
    tofEndPosition->setLabel(QString::fromLatin1("\t\tTOF End\t\t"));
	tofEndPosition->setLabelOrientation(Qt::Vertical);
    tofEndPosition->setLabelAlignment(Qt::AlignRight|Qt::AlignBottom);
    tofEndPosition->setLineStyle(QwtPlotMarker::VLine);
	tofEndPosition->setLinePen(tofPositionPen);	
    tofEndPosition->attach(this);
	tofEndPosition->setVisible(false);

	
	
	//curve for the signal
    cSignal = new QwtPlotCurve();
    //cSignal->setRenderHint(QwtPlotItem::RenderAntialiased);
    cSignal->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
	QPen signalPen(Qt::darkBlue,1,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    cSignal->setPen(signalPen);
    cSignal->attach(this);
	cSignal->setVisible(false);

	cSignalSecondary = new QwtPlotCurve();
    //cDeltaSignal->setRenderHint(QwtPlotItem::RenderAntialiased);
    cSignalSecondary->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
	QPen deltaSignalPen(Qt::red,1,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    cSignalSecondary->setPen(deltaSignalPen);
    cSignalSecondary->attach(this);
	cSignalSecondary->setVisible(false);

	// panning with the left mouse button
    (void )new QwtPlotPanner( canvas() );

    // zoom in/out with the wheel
    QwtPlotMagnifier *magnifier = new QwtPlotMagnifier( canvas() );
    magnifier->setMouseButton( Qt::NoButton );
}

void SignalPlot::clearAverages()
{
	//setting to zero will force a refresh of all averages
	numSamples=0;
}

void SignalPlot::setBaseline(double s_baselineVal)
{
	baselineVal = s_baselineVal;
	baseline->setYValue(baselineVal);
	baseline->setVisible(!logScale);
}

void SignalPlot::setGates(int start, int shortGateEnd, int longGateEnd, int tofStartPulse, int tofEndPulse)
{	
	if (displayAverage && numAveragedEvents >1)
		return;
	startPosition->setXValue(start*sampleTime);
	startPosition->setVisible(ciGatesVisible);
	shortGateEndPosition->setXValue(shortGateEnd*sampleTime);
	shortGateEndPosition->setVisible(ciGatesVisible);
	longGateEndPosition->setXValue(longGateEnd*sampleTime);
	longGateEndPosition->setVisible(ciGatesVisible);
	if (tofStartPulse>=0)
	{
		tofStartPosition->setXValue(tofStartPulse*sampleTime);
		tofStartPosition->setVisible(tofGatesVisible);
	}
	if (tofEndPulse>=0)
	{
		tofEndPosition->setXValue(tofEndPulse*sampleTime);
		tofEndPosition->setVisible(tofGatesVisible);
	}
}

void SignalPlot::setData(float* t, float* V, float* V2, int s_numSamples, float s_sampleTime, float s_offsetTime)
{
	if (s_numSamples!=numSamples)
	{
		SAFE_DELETE_ARRAY(tempT);
		SAFE_DELETE_ARRAY(tempV);
		SAFE_DELETE_ARRAY(tempV2);
		SAFE_DELETE_ARRAY(vAverage);
		SAFE_DELETE_ARRAY(vShifted);

		numSamples=s_numSamples;
		tempT=new double[numSamples];
		tempV2=new double[numSamples];
		tempV=new double[numSamples];
		vAverage=new double[numSamples];
		memset(vAverage, 0, sizeof(double)*numSamples);
		memset(tempV2, 0, sizeof(double)*numSamples);
		numAveragedEvents=0;
		vShifted=new double[numSamples];
		initOffset=s_offsetTime;
	}

	sampleTime=s_sampleTime;

	if (t)
		for (int i=0;i<numSamples;i++)
			tempT[i]=t[i];		
	else
		for (int i=0;i<numSamples;i++)	
			tempT[i]=i*sampleTime;			

	if (V)
		for (int i=0;i<numSamples;i++)
			tempV[i]=V[i];		

	if (V2)
		for (int i=0;i<numSamples;i++)
			tempV2[i]=V2[i];		


	if (V)
	{
		int shiftIndex=(s_offsetTime-initOffset)/sampleTime;
		shiftSample(V, numSamples,shiftIndex);
		for (int i=0;i<numSamples;i++)
		{
			vAverage[i]=(vAverage[i]*numAveragedEvents)+vShifted[i];
			vAverage[i]/=numAveragedEvents+1;
			vAverage[i]=fmax(0.0, vAverage[i]);
			vAverage[i]=fmin(1024.0, vAverage[i]);
		}
		numAveragedEvents++;


	}	

    //TODO: sensible axis ticks
	if (displayAverage)
		setAxisScale(xBottom, 0.0+initOffset, numSamples*sampleTime+initOffset, (numSamples*sampleTime<400)?20.0:100.0);
	else
		setAxisScale(xBottom, (alignSigs ? s_offsetTime - 200.0 : 0), numSamples*sampleTime + (alignSigs ? s_offsetTime + 100 : 0), (numSamples*sampleTime<400) ? 20.0 : 100.0);

	if (logScale)
	{
		double minVal = 1e-1;
		double maxVal = 1e3;
		setAxisScale(yLeft, minVal, maxVal);

		if (V)
			for (int i = 0; i < numSamples; i++)
				tempV[i] = fmax(baselineVal - tempV[i], minVal);

		if (V2)
			for (int i = 0; i < numSamples; i++)
				tempV2[i] = fmax(baselineVal - tempV[i], minVal);

	}

	if (V)
	{
		if (displayAverage)
			cSignal->setRawSamples(tempT, vAverage, numSamples);
		else
			cSignal->setRawSamples(tempT, tempV, numSamples);
		cSignal->setVisible(true);
	}
	else
		cSignal->setVisible(false);

	if (V2)
	{
		cSignalSecondary->setRawSamples(tempT, tempV2, numSamples);
		cSignalSecondary->setVisible(true);	
	}
	else
		cSignalSecondary->setVisible(false);

}

void SignalPlot::setTrigger(float trigger)
{
	triggerLine->setYValue(trigger);
	triggerLine->setVisible(true);
}

void SignalPlot::setDisplayAverage(bool s_displayAverage)
{
	displayAverage=s_displayAverage;
}

void SignalPlot::shiftSample(float* V, int numSamples, int shift)
{	
	//if there's a bad shift, don't shift at all
	if (std::abs(shift) >= numSamples)
	{
		memcpy(vShifted, V, sizeof(float)*numSamples);
		return;
	}
	int startPoint = std::max(-shift, 0);
	int endPoint = std::min(numSamples, numSamples - shift);
	for (int i = startPoint; i < endPoint; i++)
		vShifted[i] = V[i + shift];
	//padding
	for (int i = 0; i<startPoint; i++)
		vShifted[i] = V[0];

	for (int i = endPoint; i<numSamples; i++)
		vShifted[i] = V[numSamples - 1];
}

void SignalPlot::showCIGates(bool val) { ciGatesVisible = val; }
void SignalPlot::showToFGates(bool val) { tofGatesVisible = val; }
void SignalPlot::setAlignSignals(bool val) { alignSigs = val; }

void SignalPlot::setProjectorMode(bool projectorMode)
{
	QPen baselinePen(Qt::darkRed, 1);
	QPen tofPositionPen(Qt::darkGreen, 1);
	QPen gatePositionPen(Qt::blue, 1);
	QPen peakPositionPen(Qt::red, 1);
	QPen signalPen(Qt::darkBlue, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	QPen deltaSignalPen(Qt::red, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

	QFont currentFont = font();
	if (projectorMode)
	{
		currentFont.setPixelSize(32);
		baselinePen.setWidth(3);
		tofPositionPen.setWidth(3);
		gatePositionPen.setWidth(3);
		peakPositionPen.setWidth(3);
		signalPen.setWidth(3);
		deltaSignalPen.setWidth(3);
	}
	else
		currentFont.setPixelSize(14);
	
	QwtText yTitle = this->axisTitle(yLeft);
	yTitle.setFont(currentFont);
	QwtText xTitle = this->axisTitle(xBottom);
	xTitle.setFont(currentFont);

	setAxisTitle(yLeft, yTitle);
	setAxisTitle(xBottom, xTitle);
	
	setAxisFont(yLeft, currentFont);
	setAxisFont(xBottom, currentFont);


	baseline->setLinePen(baselinePen);
	triggerLine->setLinePen(baselinePen);
	peakPosition->setLinePen(peakPositionPen);
	halfPeakPosition->setLinePen(peakPositionPen);
	startPosition->setLinePen(gatePositionPen);
	shortGateEndPosition->setLinePen(gatePositionPen);
	longGateEndPosition->setLinePen(gatePositionPen);
	tofStartPosition->setLinePen(tofPositionPen);
	cSignal->setPen(signalPen);
	cSignalSecondary->setPen(deltaSignalPen);
}

void SignalPlot::setLogarithmicScale(bool s_logScale)
{
	logScale = s_logScale;
	if (logScale)
	{
		setAxisScaleEngine(yLeft, new QwtLogScaleEngine());
		setAxisAutoScale(yLeft, false);
	}
	else
	{
		setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
		setAxisAutoScale(QwtPlot::yLeft, true);
	}
	baseline->setVisible(!logScale);
}
