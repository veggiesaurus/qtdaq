#include "Windows/FoMWindow.h"


FoMWindow::FoMWindow (QWidget * parent, HistogramParameter s_parameterX, HistogramParameter s_parameterY, int s_chPrimary, int s_parameterMinX, int s_parameterMaxX, int s_parameterMinY, int s_parameterMaxY, int s_numBinsX, int s_numBinsY): PlotWindow(parent, s_chPrimary),
	parameterMinX(s_parameterMinX),parameterMaxX(s_parameterMaxX),parameterMinY(s_parameterMinY), parameterMaxY(s_parameterMaxY),parameterX(s_parameterX),parameterY(s_parameterY),numBinsX(s_numBinsX),numBinsY(s_numBinsY)
{
	ui.setupUi(this);	
	parameterIntervalX=((parameterMaxX-parameterMinX))/numBinsX;
	parameterIntervalY=((parameterMaxY-parameterMinY))/numBinsY;
	xVals=new double[numBinsX];
	yVals=new double[numBinsX];

	plot=ui.qwtPlotFoM;
	ui.qwtPlotFoM->setXRangeParameter(parameterMinX, parameterMaxX);
	ui.qwtPlotFoM->setAxisScale(QwtPlot::yLeft, 0, 5);
	ui.qwtPlotFoM->setAxisAutoScale(QwtPlot::yLeft, true);
	fomCurve=new QwtPlotCurve("Figure of Merit");
	fomCurve->attach(ui.qwtPlotFoM);

	 //baseline marker
	QPen effectiveSepMarkerPen(Qt::darkRed,1);
    effectiveSepMarker=new QwtPlotMarker();
    effectiveSepMarker->setLabel(QString::fromLatin1("\t\tEffective Separation\t\t"));
	effectiveSepMarker->setLabelAlignment(Qt::AlignLeft|Qt::AlignBottom);
    effectiveSepMarker->setLineStyle(QwtPlotMarker::HLine);
	effectiveSepMarker->setLinePen(effectiveSepMarkerPen);	
    effectiveSepMarker->attach(ui.qwtPlotFoM);
	effectiveSepMarker->setVisible(true);
	effectiveSepMarker->setYValue(1.27);

	tempVals=new double[numBinsY];
	A1=new double[numBinsX];
	A2=new double[numBinsX];
	x_c1=new double[numBinsX];
	x_c2=new double[numBinsX];
	sigma1=new double[numBinsX];
	sigma2=new double[numBinsX];

	ui.qwtPlotFoM->setCanvasBackground(QBrush(Qt::white));


	clearValues();
	updateTitleAndAxis();
}

FoMWindow::~FoMWindow()
{

}



void FoMWindow::onOptionsClicked()
{
	QDialog* dialog=new QDialog(this);
    uiDialogHistogramConfig.setupUi(dialog);
	uiDialogHistogramConfig.comboBoxChannel->setCurrentIndex(chPrimary);
	uiDialogHistogramConfig.comboBoxParameterX->setCurrentIndex(parameterX);
	uiDialogHistogramConfig.comboBoxParameterY->setCurrentIndex(parameterY);
	uiDialogHistogramConfig.doubleSpinBoxMinX->setValue(parameterMinX);
	uiDialogHistogramConfig.doubleSpinBoxMinY->setValue(parameterMinY);
	uiDialogHistogramConfig.doubleSpinBoxMaxX->setValue(parameterMaxX);
	uiDialogHistogramConfig.doubleSpinBoxMaxY->setValue(parameterMaxY);
	uiDialogHistogramConfig.spinBoxChannelsX->setValue(numBinsX);
	uiDialogHistogramConfig.spinBoxChannelsY->setValue(numBinsY);
    int retDialog=dialog->exec();
    if (retDialog==QDialog::Rejected)
        return;

	bool needsClear=false;
	int newChPrimary=uiDialogHistogramConfig.comboBoxChannel->currentIndex();
	if (newChPrimary!=chPrimary)
	{
		chPrimary=newChPrimary;
		needsClear=true;
		emit channelChanged(chPrimary);
	}	
	double newParameterMinX=uiDialogHistogramConfig.doubleSpinBoxMinX->value();
	double newParameterMinY=uiDialogHistogramConfig.doubleSpinBoxMinY->value();
	double newParameterMaxX=uiDialogHistogramConfig.doubleSpinBoxMaxX->value();
	double newParameterMaxY=uiDialogHistogramConfig.doubleSpinBoxMaxY->value();
	int newNumBinsX=uiDialogHistogramConfig.spinBoxChannelsX->value();
	int newNumBinsY=uiDialogHistogramConfig.spinBoxChannelsY->value();
	if (newParameterMinX!=parameterMinX || newParameterMinY!=parameterMinY || newParameterMaxX!=parameterMaxX || newParameterMaxY!=parameterMaxY || newNumBinsX!=numBinsX || newNumBinsY!=numBinsY)
	{
		parameterMinX=newParameterMinX;
		parameterMinY=newParameterMinY;
		parameterMaxX=newParameterMaxX;
		parameterMaxY=newParameterMaxY;
		parameterIntervalX=((parameterMaxX-parameterMinX))/numBinsX;
		parameterIntervalY=((parameterMaxY-parameterMinY))/numBinsY;
		ui.qwtPlotFoM->setXRangeParameter(parameterMinX, parameterMaxX);
		numBinsX=newNumBinsX;
		numBinsY=newNumBinsY;
		needsClear=true;
	}
	HistogramParameter newParameterX=(HistogramParameter)uiDialogHistogramConfig.comboBoxParameterX->currentIndex();
	HistogramParameter newParameterY=(HistogramParameter)uiDialogHistogramConfig.comboBoxParameterY->currentIndex();
	if (parameterX!=newParameterX || parameterY!=newParameterY)
	{
		parameterX=newParameterX;
		parameterY=newParameterY;
		if (parameterX!=LONG_INTEGRAL)
			ui.qwtPlotFoM->setCalibrationValues(EnergyCalibration());
		needsClear=true;		
	}
	if (needsClear)
	{
		updateTitleAndAxis();
		clearValues();
	}
}

void FoMWindow::updateTitleAndAxis()
{
	QString windowTitle;
	if (!name.isEmpty())
		windowTitle+=name+": ";
	windowTitle+="Ch "+QString::number(chPrimary);	
	windowTitle+=" Figure of Merit (";
	QString axisNameX=axisNameFromParameter(parameterX);
	QString axisNameY=axisNameFromParameter(parameterY);
	windowTitle+=axisNameX+" vs. "+axisNameY;
	windowTitle+=")";
	if (numEvents>=0)
		windowTitle+=" "+QString::number(numEvents)+" events.";	
	setWindowTitle(windowTitle);
	ui.qwtPlotFoM->setAxisTitle(QwtPlot::xBottom, axisNameX);
	ui.qwtPlotFoM->setAxisTitle(QwtPlot::yLeft, "Figure of Merit");
}

void FoMWindow::clearValues()
{
	numEvents=0;

	SAFE_DELETE_ARRAY(tempVals);
	SAFE_DELETE_ARRAY(A1);
	SAFE_DELETE_ARRAY(A2);
	SAFE_DELETE_ARRAY(x_c1);
	SAFE_DELETE_ARRAY(x_c2);
	SAFE_DELETE_ARRAY(sigma1);
	SAFE_DELETE_ARRAY(sigma2);

	tempVals=new double[numBinsY];
	A1=new double[numBinsX];
	A2=new double[numBinsX];
	x_c1=new double[numBinsX];
	x_c2=new double[numBinsX];
	sigma1=new double[numBinsX];
	sigma2=new double[numBinsX];

	memset(A1, 0, sizeof(double)*numBinsX);
	memset(A2, 0, sizeof(double)*numBinsX);
	memset(x_c1, 0, sizeof(double)*numBinsX);
	memset(x_c2, 0, sizeof(double)*numBinsX);
	memset(sigma1, 0, sizeof(double)*numBinsX);
	memset(sigma2, 0, sizeof(double)*numBinsX);

	
	if (numBinsX && numBinsY)
	{
		values.clear();
		values.resize(numBinsX*numBinsY);
		values.fill(0);
	}
	fomCurve->setSamples(NULL, NULL, 0);
	ui.qwtPlotFoM->replot();
}
void FoMWindow::onRefreshClicked()
{
	
	if (numBinsX && numBinsY)
	{
		SAFE_DELETE_ARRAY(xVals);
		SAFE_DELETE_ARRAY(yVals);
		xVals=new double[numBinsX];
		yVals=new double[numBinsX];
		for (int i=0;i<numBinsX;i++)
		{
			xVals[i]=parameterMinX+i*parameterIntervalX;
			yVals[i]=getFigureOfMerit(i);
		}
		fomCurve->setSamples(xVals, yVals, numBinsX);
	}
	ui.qwtPlotFoM->replot();
	updateTitleAndAxis();
}

void FoMWindow::onNewCalibration(int channel, EnergyCalibration calibration)
{
	if (channel==chPrimary && calibration.calibrated && parameterX==LONG_INTEGRAL)
		ui.qwtPlotFoM->setCalibrationValues(calibration);
}

void FoMWindow::onNewEventStatistics(QVector<EventStatistics*>* events)
{
	 if (!events)
        return;
	 
	 parameterIntervalX=(parameterMaxX-parameterMinX)/numBinsX;
	 parameterIntervalY=(parameterMaxY-parameterMinY)/numBinsY;

    for (QVector<EventStatistics*>::iterator it=events->begin();it!=events->end();it++)
	{
		EventStatistics* eventStats=*it;
		SampleStatistics stats=eventStats->channelStatistics[chPrimary];
		
		
		int channelNum=stats.channelNumber;
		//skip events with mismatched channels or outside cut region
		if (channelNum!=chPrimary || !isInActiveRegion(eventStats))
			continue;

		double valX=valueFromStatsAndParameter(stats, parameterX);
		double valY=valueFromStatsAndParameter(stats, parameterY);		
		
		int indexX=(int)((valX-parameterMinX)/parameterIntervalX);
		int indexY=(int)((valY-parameterMinY)/parameterIntervalY);
		if (indexX>=0 && indexX<numBinsX && indexY>=0 && indexY<numBinsY)
		{
			values[indexX+numBinsX*indexY]++;	
			numEvents++;
		}
		else
			numEvents++;
	}
}

void FoMWindow::onSaveDataClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set export file", "", "Text File (*.txt);;All files (*.*)");
	fileDialog.restoreState(settings.value("plotWindow/saveDataState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);

	if (!fileDialog.exec())
		return;
	settings.setValue("plotWindow/saveDataState", fileDialog.saveState());
	QString fileName=fileDialog.selectedFiles().first();
	QFile exportFile(fileName);
	if (!exportFile.open(QIODevice::WriteOnly))
		return;
	
	QString headingX, headingY;
	headingY="Figure of Merit";
	headingX=axisNameFromParameter(parameterX);

	QTextStream stream( &exportFile );
	stream<<headingX<<" \t"<<headingY<<endl;
	int numEntries=numBinsX;
	for (int i=0;i<numEntries;i++)
	{		
		stream<<xVals[i]<<" \t"<<yVals[i]<<endl;
	}
	stream.flush();
	exportFile.close();
}


double FoMWindow::getFigureOfMerit(int indexX)
{
	int numEventsInBin=0;
	for (int i=0;i<numBinsY;i++)
	{
		int binVal=values[indexX+numBinsX*i];
		numEventsInBin+=binVal;
		tempVals[i]=binVal;
	}


	int indexMax=0;
	double maxVal=0;

	double normNewGuess=0;
	double normPrevGuess=0;
	double normAdjGuess=0;	
	double fom=0;

	//can't fit fewer than 100 entries
	if (numEventsInBin<100)
		return fom;
	//finds index of max val
	for (int i=0;i<numBinsY;i++)
	{
		indexMax=(tempVals[i]>maxVal?i:indexMax);
		maxVal=max(maxVal, tempVals[i]);
	}

	//dummy initial guesses
	double initA1=maxVal, initX_C1=indexMax, initSigma1=1, initA2=maxVal/2.0, initX_C2=numBinsY*.9, initSigma2=1;
	initialGuesses(numBinsY, tempVals, parameterMinY, parameterMaxY, initA1, initA2, initX_C1, initX_C2, initSigma1, initSigma2);	
	double* paramsNewGuess=new double[6];
	double* paramsPrevGuess=new double[6];
	double* paramsAdjGuess=new double[6];

	bool fitSuccessNewGuess=fitDoubleGaussian(numBinsY, tempVals, initA1, initSigma1, initX_C1, initA2, initSigma2, initX_C2, paramsNewGuess, normNewGuess);
	bool fitSuccessPrevGuess=fitDoubleGaussian(numBinsY, tempVals, maxVal, sigma1[indexX], x_c1[indexX], (A2[indexX]/A1[indexX])*maxVal, sigma2[indexX], x_c2[indexX], paramsPrevGuess, normPrevGuess);
	bool fitSuccessAdjGuess=false;
	if (indexX>1);
		fitSuccessAdjGuess=fitDoubleGaussian(numBinsY, tempVals, maxVal, sigma1[indexX-1], x_c1[indexX-1], A2[indexX-1], sigma2[indexX-1], x_c2[indexX-1], paramsAdjGuess, normAdjGuess);
	
	normNewGuess=(fitSuccessNewGuess?normNewGuess:INT_MAX);
	normPrevGuess=(fitSuccessPrevGuess?normPrevGuess:INT_MAX);
	normAdjGuess=(fitSuccessAdjGuess?normAdjGuess:INT_MAX);

	//if there's a success
	if (fitSuccessAdjGuess || fitSuccessNewGuess || fitSuccessPrevGuess)
	{
		double* newParams;
		if (normNewGuess<=normPrevGuess && normNewGuess<=normAdjGuess)
			newParams=paramsNewGuess;
		else if (normPrevGuess<=normAdjGuess && normPrevGuess<=normNewGuess)
			newParams=paramsPrevGuess;
		else
			newParams=paramsAdjGuess;

		A1[indexX]=abs(newParams[0]);
		sigma1[indexX]=abs(newParams[1]);
		x_c1[indexX]=newParams[2];

		A2[indexX]=abs(newParams[3]);
		sigma2[indexX]=abs(newParams[4]);
		x_c2[indexX]=newParams[5];
		fom=abs(x_c1[indexX]-x_c2[indexX])/(2.35482*(sigma1[indexX]+sigma2[indexX]));
	}
	SAFE_DELETE_ARRAY(paramsNewGuess);
	SAFE_DELETE_ARRAY(paramsPrevGuess);
	SAFE_DELETE_ARRAY(paramsAdjGuess);
	//error checking: 0<=FoM<5
	if (fom<0 || fom>5)
		fom=0;
	return fom;
}

void FoMWindow::timerUpdate()
{

}

void FoMWindow::updateSettings()
{	
	parameterIntervalX=(parameterMaxX-parameterMinX)/numBinsX;
	parameterIntervalY=(parameterMaxY-parameterMinY)/numBinsY;
	ui.qwtPlotFoM->setXRangeParameter(parameterMinX, parameterMaxX);
	updateTitleAndAxis();
}

QDataStream &operator<<(QDataStream &out, const FoMWindow &obj)
{
	out<<static_cast<const PlotWindow&>(obj);
	out<<UI_SAVE_VERSION;	
	quint32 parmX=obj.parameterX, parmY=obj.parameterY;
	out<<parmX<<parmY<<obj.numBinsX<<obj.numBinsY;
	out<<obj.parameterMinX<<obj.parameterMaxX<<obj.parameterMinY<<obj.parameterMaxY;

	return out;	
}

QDataStream &operator>>(QDataStream &in, FoMWindow &obj)
{
	in>>static_cast<PlotWindow&>(obj);
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	quint32 parmX, parmY;
	in>>*((quint32*)&obj.parameterX);
	in>>*((quint32*)&obj.parameterY);
	in>>obj.numBinsX>>obj.numBinsY;
	in>>obj.parameterMinX>>obj.parameterMaxX>>obj.parameterMinY>>obj.parameterMaxY;
	
	obj.updateSettings();
	return in;
}