#include "Plots/signalplot.h"


SignalPlot::SignalPlot(QWidget *parent):
    QwtPlot( parent )
{
    setAutoReplot(false);

    // axes
    setAxisTitle(xBottom, "t (ns)" );

    setAxisTitle(yLeft, "ADC Count");
	//TODO: this is for deltas
    setAxisScale(yLeft, 0, 1024);
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

	cDeltaSignal = new QwtPlotCurve();
    //cDeltaSignal->setRenderHint(QwtPlotItem::RenderAntialiased);
    cDeltaSignal->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
	QPen deltaSignalPen(Qt::red,1,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    cDeltaSignal->setPen(deltaSignalPen);
    cDeltaSignal->attach(this);
	cDeltaSignal->setVisible(false);

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

void SignalPlot::setBaseline(double baselineVal)
{
	baseline->setYValue(baselineVal);
	baseline->setVisible(true);
}

void SignalPlot::setGates(int start, int shortGateEnd, int longGateEnd, int tofStartPulse, int tofEndPulse)
{	
	if (displayAverage && numAveragedEvents >1)
		return;
	startPosition->setXValue(start*sampleTime);
	startPosition->setVisible(true);
	shortGateEndPosition->setXValue(shortGateEnd*sampleTime);
	shortGateEndPosition->setVisible(true);
	longGateEndPosition->setXValue(longGateEnd*sampleTime);
	longGateEndPosition->setVisible(true);
	if (tofStartPulse>=0)
	{
		tofStartPosition->setXValue(tofStartPulse*sampleTime);
		tofStartPosition->setVisible(true);
	}
	if (tofEndPulse>=0)
	{
		tofEndPosition->setXValue(tofEndPulse*sampleTime);
		tofEndPosition->setVisible(true);
	}
}

void SignalPlot::setData(float* t, float* V, float* DV, int s_numSamples, float s_sampleTime, float s_offsetTime)
{
	if (s_numSamples!=numSamples)
	{
		SAFE_DELETE_ARRAY(tempT);
		SAFE_DELETE_ARRAY(tempV);
		SAFE_DELETE_ARRAY(tempDV);
		SAFE_DELETE_ARRAY(vAverage);
		SAFE_DELETE_ARRAY(vShifted);

		numSamples=s_numSamples;
		tempT=new double[numSamples];
		tempDV=new double[numSamples];
		tempV=new double[numSamples];
		vAverage=new double[numSamples];
		memset(vAverage, 0, sizeof(double)*numSamples);
		memset(tempDV, 0, sizeof(double)*numSamples);
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

	if (DV)
		for (int i=0;i<numSamples;i++)
			tempDV[i]=DV[i];		


	if (V)
	{
		int shiftIndex=(s_offsetTime-initOffset)/sampleTime;
		shiftSample(V, numSamples,shiftIndex);
		for (int i=0;i<numSamples;i++)
		{
			vAverage[i]=(vAverage[i]*numAveragedEvents)+vShifted[i];
			vAverage[i]/=numAveragedEvents+1;
			vAverage[i]=max(0.0, vAverage[i]);
			vAverage[i]=min(1024.0, vAverage[i]);
		}
		numAveragedEvents++;
	}

    //TODO: sensible axis ticks
	if (displayAverage)
		setAxisScale(xBottom, 0.0+initOffset, numSamples*sampleTime+initOffset, (numSamples*sampleTime<400)?20.0:100.0);
	else
		setAxisScale(xBottom, -200.0+s_offsetTime, numSamples*sampleTime+s_offsetTime+100, (numSamples*sampleTime<400)?20.0:100.0);


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

	if (DV)
	{
		cDeltaSignal->setRawSamples(tempT, tempDV, numSamples);
		cDeltaSignal->setVisible(true);	
	}
	else
		cDeltaSignal->setVisible(false);

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
	for (int i=max(-shift, 0);i<min(numSamples, numSamples+shift)-1;i++) 
		vShifted[i]=V[i+shift];

	//padding
	for (int i=0;i<-shift;i++)
		vShifted[i]=V[0];

	for (int i=numSamples+shift-1;i<numSamples-1;i++)
		vShifted[i]=V[numSamples-1];
}