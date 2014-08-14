#include "HistogramWindow.h"



HistogramWindow::HistogramWindow (QWidget * parent, HistogramParameter s_parameter, int s_chPrimary, double s_parameterMin, double s_parameterMax, int s_numBins, bool s_delta, bool s_logScale): PlotWindow(parent, s_chPrimary),
	parameter(s_parameter), parameterMin(s_parameterMin), parameterMax(s_parameterMax), numBins(s_numBins), delta(s_delta), logScale(s_logScale)
{
	ui.setupUi(this);
	binVals=NULL;
	displayMeanAndFWHM=displayFoM=false;
	autoFitGaussian=autoFitDoubleGaussian=false;

	FWHM=0;
	mean=0;

	plot=ui.qwtPlotHistogram;
	ui.qwtPlotHistogram->setLogScale(logScale);
	
	ui.actionLogarithmicScale->setChecked(logScale);
	ui.actionDeltaHistogram->setChecked(delta);
	
	ui.qwtPlotHistogram->setXRangeParameter(parameterMin, parameterMax);
	ui.qwtPlotHistogram->setCanvasBackground(QBrush(Qt::white));
	recreateHistogram();
	updateTitleAndAxis();
}

HistogramWindow::~HistogramWindow()
{
	SAFE_DELETE_ARRAY(binVals);
}

void HistogramWindow::onOptionsClicked()
{
	QDialog* dialog=new QDialog(this);
    uiDialogHistogramConfig.setupUi(dialog);
	uiDialogHistogramConfig.comboBoxChannel->setCurrentIndex(chPrimary);
	uiDialogHistogramConfig.comboBoxParameter->setCurrentIndex(parameter);
	uiDialogHistogramConfig.doubleSpinBoxMin->setValue(parameterMin);
	uiDialogHistogramConfig.doubleSpinBoxMax->setValue(parameterMax);
	uiDialogHistogramConfig.spinBoxChannels->setValue(numBins);

    int retDialog=dialog->exec();
	if (retDialog == QDialog::Rejected)
	{
		SAFE_DELETE(dialog);
		return;
	}

	bool needsClear=false;

	int newChPrimary=uiDialogHistogramConfig.comboBoxChannel->currentIndex();
	if (newChPrimary!=chPrimary)
	{
		chPrimary=newChPrimary;
		needsClear=true;
		emit channelChanged(chPrimary);
	}	
	double newParameterMin=uiDialogHistogramConfig.doubleSpinBoxMin->value();
	double newParameterMax=uiDialogHistogramConfig.doubleSpinBoxMax->value();
	int newNumBins=uiDialogHistogramConfig.spinBoxChannels->value();
	if (newParameterMin!=parameterMin || newParameterMax!=parameterMax || newNumBins!=numBins)
	{
		parameterMin=newParameterMin;
		parameterMax=newParameterMax;
		ui.qwtPlotHistogram->setXRangeParameter(parameterMin, parameterMax);
		numBins=newNumBins;
		needsClear=true;
	}
	HistogramParameter newParameter=(HistogramParameter)uiDialogHistogramConfig.comboBoxParameter->currentIndex();
	if (parameter!=newParameter)
	{
		parameter=newParameter;
		if (parameter!=LONG_INTEGRAL)
			ui.qwtPlotHistogram->setCalibrationValues(EnergyCalibration());
		needsClear=true;		
	}
	if (needsClear)
	{
		updateTitleAndAxis();
		recreateHistogram();
	}
	SAFE_DELETE(dialog);
}

void HistogramWindow::onLogScaleToggled(bool logScaleEnabled)
{
	logScale=logScaleEnabled;
	ui.qwtPlotHistogram->setLogScale(logScale);
}

void HistogramWindow::onDeltaHistogramToggled(bool deltaEnabled)
{
	delta=deltaEnabled;
}

void HistogramWindow::onDuplicateClicked()
{
	HistogramWindow* hist=new HistogramWindow(parentWidget(), parameter, chPrimary, parameterMin, parameterMax, numBins, delta, logScale);
	hist->updateSettings();
	hist->conditions=conditions;
	hist->polygonalConditions=polygonalConditions;	
	emit duplicate(hist);
}

void HistogramWindow::onAddLinearCutClicked()
{
	LinearCutDialog cutDlg(this);
	cutDlg.ui.comboBoxChannel->setCurrentIndex(chPrimary+1);
	cutDlg.ui.comboBoxParameter->setCurrentIndex(parameter);	
	if (cutDlg.exec()==QDialog::Accepted)
	{
		LinearCut cut;
		cut.channel=cutDlg.ui.comboBoxChannel->currentIndex()-1;
		cut.parameter=(HistogramParameter)cutDlg.ui.comboBoxParameter->currentIndex();
		cut.cutMin=cutDlg.ui.doubleSpinBoxMin->value();
		cut.cutMax=cutDlg.ui.doubleSpinBoxMax->value();
		cut.name=cutDlg.ui.lineEditName->text();
		emit linearCutCreated(cut);
	}
}


void HistogramWindow::updateTitleAndAxis()
{
	QString windowTitle;
	if (!name.isEmpty())
		windowTitle+=name+": ";
	windowTitle+="Ch "+QString::number(chPrimary);	
	windowTitle+=" Histogram (";
	QString axisName=axisNameFromParameter(parameter);
	windowTitle+=axisName;
	windowTitle+=")";
	if (numEvents>=0)
		windowTitle+=" "+QString::number(numEvents)+" events.";
	if (displayMeanAndFWHM)
		windowTitle+=" Mean: "+QString::number(mean)+", FWHM: "+QString::number(FWHM);
	else if (displayFoM)
		windowTitle+=" Figure of Merit: "+QString::number(fom);
	setWindowTitle(windowTitle);
	ui.qwtPlotHistogram->setAxisTitle(QwtPlot::xBottom, axisName);
}

void HistogramWindow::recreateHistogram()
{
	SAFE_DELETE_ARRAY(binVals);
	if (numBins)
	{
		binVals=new double[numBins];
		//for log purposes
		for (int i=0;i<numBins;i++)
			binVals[i]=LOG_MIN;				
		ui.qwtPlotHistogram->setValues(numBins, binVals, parameterMin, parameterMax);
	}
}

void HistogramWindow::clearValues()
{
	numEvents=0;
	recreateHistogram();
}
void HistogramWindow::timerUpdate()
{
	if (numBins && binVals)
	{		
		//need to manually update scale if log
		if (logScale)
		{
			double maxVal=1; 
			for (int i=0;i<numBins;i++)
				maxVal=max(maxVal, binVals[i]);
			ui.qwtPlotHistogram->setAxisScale(QwtPlot::yLeft, 0.1, maxVal*2);
		}

		if (!delta)
			ui.qwtPlotHistogram->setValues(numBins, binVals, parameterMin, parameterMax);
		else
		{
			double* deltaBins=new double[numBins];
			memset(deltaBins, 0, sizeof(double)*numBins);

			//Quadratic 19-window Savitzky-Golay differentiating filter (not correctly normalised!)
			int halfWindowLength=9;
			for (int i=halfWindowLength;i<numBins-halfWindowLength;i++)
			{
				for (int j=1;j<=halfWindowLength;j++)
				{
					deltaBins[i]+=-j*binVals[i-j];
					deltaBins[i]+=j*binVals[i+j];
				}
				deltaBins[i]/=583.0;
			}
			ui.qwtPlotHistogram->setValues(numBins, deltaBins, parameterMin, parameterMax);
			delete[] deltaBins;
		}
	}

	if (autoFitGaussian)
		onFitGaussianClicked();
	else if (autoFitDoubleGaussian)
		onFitDoubleGaussianClicked();
	ui.qwtPlotHistogram->replot();
	updateTitleAndAxis();
}

void HistogramWindow::onNewCalibration(int channel, EnergyCalibration calibration)
{
	if (channel==chPrimary && calibration.calibrated && parameter==LONG_INTEGRAL)
		ui.qwtPlotHistogram->setCalibrationValues(calibration);
}

void HistogramWindow::onNewEventStatistics(QVector<EventStatistics*>* events)
{
	 if (!events)
        return;
	 
	parameterInterval=(parameterMax-parameterMin)/numBins;

    for (QVector<EventStatistics*>::iterator it=events->begin();it!=events->end();it++)
	{
		EventStatistics* eventStats=*it;
		SampleStatistics stats=eventStats->channelStatistics[chPrimary];
		
		
		int channelNum=stats.channelNumber;
		//skip events with mismatched channels or outside cut region
		if (channelNum!=chPrimary || !isInActiveRegion(eventStats))
			continue;
	
		double val=valueFromStatsAndParameter(stats, parameter);		
		
		int index=(int)((val-parameterMin)/parameterInterval);
		if (index>=0 && index<numBins)
		{
			binVals[index]++;
			numEvents++;
		}
		else
		{
			numEvents++;
		}		
	}
	timerUpdate();
}

void HistogramWindow::onSaveDataClicked()
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
	
	QwtSeriesData<QwtIntervalSample>* data=ui.qwtPlotHistogram->getValues();
	QString headingX, headingY;
	headingY="Counts";
	headingX=axisNameFromParameter(parameter);

	QTextStream stream( &exportFile );
	stream<<headingX<<" \t"<<headingY<<endl;
	int numEntries=data->size();
	for (int i=0;i<numEntries;i++)
	{
		QwtIntervalSample sample=data->sample(i);
		stream<<sample.interval.minValue()<<" \t"<<sample.value<<endl;
	}
	stream.flush();
	exportFile.close();
}


void HistogramWindow::onFitGaussianClicked()
{
	int indexMax=0;
	double maxVal=0;
	double norm=0;
	//finds index of max val
	for (int i=0;i<numBins;i++)
	{
		indexMax=(binVals[i]>maxVal?i:indexMax);
		maxVal=max(maxVal, binVals[i]);
	}
	//10% of x_c as initial guess
	double initSigma=max(indexMax, 1)*0.1;
	double* params=new double[3];
	double* paramErrors=new double[3];
	bool fitSuccess=fitGaussian(numBins, binVals, maxVal, initSigma, indexMax, params, paramErrors, norm);
	if (fitSuccess)
	{
		double fittedA=abs(params[0]);
		double fittedSigma=abs(params[1]);
		double fittedX_C=params[2];
		double u_A=paramErrors[0];
		double u_sigma=paramErrors[1];
		double u_x_c=paramErrors[2];
		int numFittedPoints=numBins*10;
		double* xVals=new double[numFittedPoints];
		double* yVals=new double[numFittedPoints];
		double intervalFittedPoints=(parameterMax-parameterMin)/numFittedPoints;
		for (int i=0;i<numFittedPoints;i++)
		{
			xVals[i]=parameterMin+intervalFittedPoints*(i+5);
			double index=i/10.0;
			yVals[i]=fittedA*exp(-0.5*pow((index-fittedX_C)/fittedSigma, 2));			
		}
		ui.qwtPlotHistogram->setFittedValues(numFittedPoints, xVals, yVals);
		FWHM=2.35482*fittedSigma*parameterInterval;
		double u_FWHM=2.35482*u_sigma*parameterInterval;
		mean=parameterMin+parameterInterval*fittedX_C;
		double u_mean=parameterInterval*u_x_c;
		displayMeanAndFWHM=true;
		displayFoM=false;
		autoFitGaussian=true;
		autoFitDoubleGaussian=false;
		SAFE_DELETE_ARRAY(xVals);
		SAFE_DELETE_ARRAY(yVals);
	}
	SAFE_DELETE_ARRAY(params);
	SAFE_DELETE_ARRAY(paramErrors);


}
void HistogramWindow::onFitDoubleGaussianClicked()
{
	int indexMax=0;
	double maxVal=0;

	double normNewGuess=0;
	double normPrevGuess=0;
	autoFitDoubleGaussian=true;
	autoFitGaussian=false;

	//can't fit fewer than 100 entries
	if (numEvents<100)
		return;
	//finds index of max val
	for (int i=0;i<numBins;i++)
	{
		indexMax=(binVals[i]>maxVal?i:indexMax);
		maxVal=max(maxVal, binVals[i]);
	}

	//dummy initial guesses
	double initA1=maxVal, initX_C1=indexMax, initSigma1=1, initA2=maxVal/2.0, initX_C2=numBins*.9, initSigma2=1;
	initialGuesses(numBins, binVals, parameterMin, parameterMax, initA1, initA2, initX_C1, initX_C2, initSigma1, initSigma2);	
	double* paramsNewGuess=new double[6];
	double* paramsPrevGuess=new double[6];

	bool fitSuccessNewGuess=fitDoubleGaussian(numBins, binVals, initA1, initSigma1, initX_C1, initA2, initSigma2, initX_C2, paramsNewGuess, normNewGuess);
	bool fitSuccessPrevGuess=fitDoubleGaussian(numBins, binVals, A1, sigma1, x_c1, A2, sigma2, x_c2, paramsPrevGuess, normPrevGuess);
	
	//new starting point is better than prev
	if ((fitSuccessNewGuess && !fitSuccessPrevGuess) || normNewGuess<normPrevGuess)
	{
		A1=abs(paramsNewGuess[0]);
		sigma1=abs(paramsNewGuess[1]);
		x_c1=paramsNewGuess[2];

		A2=abs(paramsNewGuess[3]);
		sigma2=abs(paramsNewGuess[4]);
		x_c2=paramsNewGuess[5];
	}
	else
	{
		A1=abs(paramsPrevGuess[0]);
		sigma1=abs(paramsPrevGuess[1]);
		x_c1=paramsPrevGuess[2];

		A2=abs(paramsPrevGuess[3]);
		sigma2=abs(paramsPrevGuess[4]);
		x_c2=paramsPrevGuess[5];
	}
	

	if (fitSuccessNewGuess  || fitSuccessPrevGuess)
	{		
		int numFittedPoints=numBins*10;
		double* xVals=new double[numFittedPoints];
		double* yVals=new double[numFittedPoints];
		double intervalFittedPoints=(parameterMax-parameterMin)/numFittedPoints;
		for (int i=0;i<numFittedPoints;i++)
		{
			//half way between
			xVals[i]=parameterMin+intervalFittedPoints*(i+5);
			double index=i/10.0;
			yVals[i]=A1*exp(-0.5*pow((index-x_c1)/sigma1, 2));
			yVals[i]+=A2*exp(-0.5*pow((index-x_c2)/sigma2, 2));
		}
		ui.qwtPlotHistogram->setFittedValues(numFittedPoints, xVals, yVals);
		fom=abs(x_c1-x_c2)/(2.35482*(sigma1+sigma2));
		mean=parameterMin+parameterInterval*x_c1;
		displayMeanAndFWHM=false;
		displayFoM=true;
		
		SAFE_DELETE_ARRAY(xVals);
		SAFE_DELETE_ARRAY(yVals);
	}
	SAFE_DELETE_ARRAY(paramsNewGuess);
	SAFE_DELETE_ARRAY(paramsPrevGuess);
}

void HistogramWindow::updateSettings()
{
	parameterInterval=(parameterMax-parameterMin)/numBins;
	ui.qwtPlotHistogram->setXRangeParameter(parameterMin, parameterMax);
	ui.actionDeltaHistogram->setChecked(delta);
	ui.actionLogarithmicScale->setChecked(logScale);
	ui.qwtPlotHistogram->setLogScale(logScale);		
	recreateHistogram();
	updateTitleAndAxis();
}

QDataStream &operator<<(QDataStream &out, const HistogramWindow &obj)
{
	out<<static_cast<const PlotWindow&>(obj);
	out<<UI_SAVE_VERSION;	
	quint32 parm=obj.parameter;
	out<<parm<<obj.numBins;
	out<<obj.parameterMin<<obj.parameterMax;
	out<<obj.delta<<obj.logScale;
	return out;	
}

QDataStream &operator>>(QDataStream &in, HistogramWindow &obj)
{
	in>>static_cast<PlotWindow&>(obj);
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	in>>*((quint32*)&obj.parameter);
	in>>obj.numBins;
	in>>obj.parameterMin>>obj.parameterMax;
	in>>obj.delta>>obj.logScale;

	obj.updateSettings();
	return in;
}