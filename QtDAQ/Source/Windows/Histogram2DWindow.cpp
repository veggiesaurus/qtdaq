#include "Histogram2DWindow.h"


Histogram2DWindow::Histogram2DWindow (QWidget * parent, HistogramParameter s_parameterX, HistogramParameter s_parameterY, int s_chPrimary, double s_parameterMinX, double s_parameterMaxX, double s_parameterMinY, double s_parameterMaxY, int s_numBinsX, int s_numBinsY, bool s_logScale, bool s_smoothing): PlotWindow(parent, s_chPrimary),
	parameterMinX(s_parameterMinX),parameterMaxX(s_parameterMaxX),parameterMinY(s_parameterMinY), parameterMaxY(s_parameterMaxY),parameterX(s_parameterX),parameterY(s_parameterY),numBinsX(s_numBinsX),numBinsY(s_numBinsY),
	logScale(s_logScale),smoothing(s_smoothing)
{
	ui.setupUi(this);
	
	connect(ui.qwtPlotHistogram2D, SIGNAL(pointSelectionComplete(const QVector< QPointF >&)), this, SLOT(onPolygonPointsSelected(const QVector< QPointF >&)));
	
	chPrimary=s_chPrimary;	
	plot=ui.qwtPlotHistogram2D;
	ui.qwtPlotHistogram2D->setCanvasBackground(QBrush(Qt::white));
	ui.qwtPlotHistogram2D->setXRangeParameter(parameterMinX, parameterMaxX);
	ui.qwtPlotHistogram2D->setYRange(parameterMinY, parameterMaxY);
	ui.qwtPlotHistogram2D->setLogScale(logScale);
	ui.qwtPlotHistogram2D->picker->setTrackerPen(QPen(Qt::black));
	ui.actionLogarithmicColourScale->setChecked(logScale);
	ui.actionBilinearSmoothing->setChecked(smoothing);
	
	recreateHistogram();
	updateTitleAndAxis();

	dialogPolygonalCutEdit=NULL;

}

Histogram2DWindow::~Histogram2DWindow()
{

}



void Histogram2DWindow::onOptionsClicked()
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
		ui.qwtPlotHistogram2D->setXRangeParameter(parameterMinX, parameterMaxX);
		ui.qwtPlotHistogram2D->setYRange(parameterMinY, parameterMaxY);
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
		needsClear=true;		
	}
	if (needsClear)
	{
		updateTitleAndAxis();
		recreateHistogram();
	}
}

void Histogram2DWindow::updateTitleAndAxis()
{
	QString windowTitle;
	if (!name.isEmpty())
		windowTitle+=name+": ";
	windowTitle+="Ch "+QString::number(chPrimary);	
	windowTitle+=" Histogram (";
	QString axisNameX=axisNameFromParameter(parameterX);
	QString axisNameY=axisNameFromParameter(parameterY);
	windowTitle+=axisNameX+" vs. "+axisNameY;
	windowTitle+=")";
	if (numEvents>=0)
		windowTitle+=" "+QString::number(numEvents)+" events.";	
	setWindowTitle(windowTitle);
	ui.qwtPlotHistogram2D->setAxisTitle(QwtPlot::xBottom, axisNameX);
	ui.qwtPlotHistogram2D->setAxisTitle(QwtPlot::yLeft, axisNameY);
}

void Histogram2DWindow::recreateHistogram()
{
	if (numBinsX && numBinsY)
	{		
		values.resize(numBinsX*numBinsY);
		values.fill(0);
		ui.qwtPlotHistogram2D->setRasterData(values, numBinsX, parameterMinX, parameterMaxX, parameterMinY, parameterMaxY, 0, 1);
	}
}



void Histogram2DWindow::clearValues()
{
	numEvents=0;
	recreateHistogram();
}
void Histogram2DWindow::timerUpdate()
{
	if (numBinsX && numBinsY)
	{
		QVector<double>::iterator itValues = std::max_element(values.begin(), values.end());
		//TODO: this scaling should come from options!
		double maxVal=(*itValues);
		ui.qwtPlotHistogram2D->setRasterData(values, numBinsX, parameterMinX, parameterMaxX, parameterMinY, parameterMaxY, 0, maxVal);
		ui.qwtPlotHistogram2D->setResampleMode(smoothing?QwtMatrixRasterData::BilinearInterpolation:QwtMatrixRasterData::NearestNeighbour);
	}
	ui.qwtPlotHistogram2D->replot();
	updateTitleAndAxis();
}

void Histogram2DWindow::onNewCalibration(int channel, EnergyCalibration calibration)
{
	if (channel==chPrimary && calibration.calibrated)
		ui.qwtPlotHistogram2D->setCalibrationValues(calibration);
}

void Histogram2DWindow::onAddPolygonalCutClicked()
{
	//move out of edit mode
	if (dialogPolygonalCutEdit)
	{
		dialogPolygonalCutEdit->disconnect();
		SAFE_DELETE(dialogPolygonalCutEdit);		
	}

	ui.qwtPlotHistogram2D->enablePolygonPicker(true);
}

void Histogram2DWindow::onEditPolygonalCutClicked()
{
	if (dialogPolygonalCutEdit)
	{
		dialogPolygonalCutEdit->disconnect();
		SAFE_DELETE(dialogPolygonalCutEdit);		
	}
	dialogPolygonalCutEdit=new PolygonalCutEditDialog(*polygonalCuts, false, this);
	connect(dialogPolygonalCutEdit, SIGNAL(selectedCutChanged(QPolygonF)), this, SLOT(onPolygonalCutEditSelectionChanged(QPolygonF)));
	connect(dialogPolygonalCutEdit, SIGNAL(redrawClicked()), this, SLOT(onPolygonalCutEditRedrawRequest()));
	connect(dialogPolygonalCutEdit, SIGNAL(accepted()), this, SLOT(onPolygonalCutEditAccepted()));
	dialogPolygonalCutEdit->exec();		
}

void Histogram2DWindow::onRemovePolygonalCutClicked()
{
	PolygonalCutEditDialog cutDlg(*polygonalCuts, true, this);
	connect(&cutDlg, SIGNAL(selectedCutChanged(QPolygonF)), this, SLOT(onPolygonalCutEditSelectionChanged(QPolygonF)));	
	if (cutDlg.exec()==QDialog::Accepted && (*polygonalCuts).size())
	{
		int cutNum=cutDlg.ui.comboBoxName->currentIndex();
		emit cutRemoved(cutNum, true);
		//remove drawn polg
		ui.qwtPlotHistogram2D->setSelectedPolygon(QPolygonF());
	}
}

void Histogram2DWindow::onAddLinearCutClicked()
{
	LinearCutDialog cutDlg(this);
	cutDlg.ui.comboBoxChannel->setCurrentIndex(chPrimary+1);
	cutDlg.ui.comboBoxParameter->setCurrentIndex(parameterX);	
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

void Histogram2DWindow::onPolygonalCutEditSelectionChanged(QPolygonF poly)
{
	ui.qwtPlotHistogram2D->setSelectedPolygon(poly);
	ui.qwtPlotHistogram2D->replot();
}

void Histogram2DWindow::onPolygonPointsSelected(const QVector< QPointF >&pa)
{		
	ui.qwtPlotHistogram2D->displayCutPolygon(true);

	if (dialogPolygonalCutEdit)
	{
		dialogPolygonalCutEdit->onRedrawComplete(pa);
		return;
	}

	PolygonalCutDialog dialog(chPrimary, parameterX, parameterY, this);
    int retDialog=dialog.exec();
    if (retDialog==QDialog::Rejected)
	{
        ui.qwtPlotHistogram2D->enablePolygonPicker(false);
		ui.qwtPlotHistogram2D->displayCutPolygon(false);
	}
	//redraw: -1
	else if (retDialog==-1)
	{
		ui.qwtPlotHistogram2D->enablePolygonPicker(true);
	}
	else if (retDialog==QDialog::Accepted)
	{
		ui.qwtPlotHistogram2D->displayCutPolygon(true);
        ui.qwtPlotHistogram2D->enablePolygonPicker(false);
		//add cut
		PolygonalCut cut;
		cut.channel=dialog.ui.comboBoxChannel->currentIndex()-1;
		cut.name=dialog.ui.lineEditName->text();
		cut.parameterX=parameterX;
		cut.parameterY=parameterY;
		cut.points=pa;
		emit(polygonalCutCreated(cut));
	}
}

void Histogram2DWindow::onPolygonalCutEditRedrawRequest()
{
	ui.qwtPlotHistogram2D->enablePolygonPicker(true);
}

void Histogram2DWindow::onPolygonalCutEditAccepted()
{
	dialogPolygonalCutEdit->updateCut();
	dialogPolygonalCutEdit->disconnect();
	SAFE_DELETE(dialogPolygonalCutEdit);
}

void Histogram2DWindow::onDuplicateClicked()
{
	Histogram2DWindow* hist=new Histogram2DWindow(parentWidget(), parameterX, parameterY, chPrimary, parameterMinX, parameterMaxX, parameterMinY, parameterMaxY, numBinsX, numBinsY, logScale, smoothing);
	hist->updateSettings();
	hist->conditions=conditions;
	hist->polygonalConditions=polygonalConditions;

	emit duplicate(hist);
}

void Histogram2DWindow::onLogColoursToggled(bool logColoursEnabled)
{
	logScale=logColoursEnabled;
	ui.qwtPlotHistogram2D->setLogScale(logScale);
}

void Histogram2DWindow::onBilinearSmoothingToggled(bool smoothingEnabled)
{
	smoothing=smoothingEnabled;
}

void Histogram2DWindow::onNewEventStatistics(QVector<EventStatistics*>* events)
{
	 if (!events)
        return;
	 
	 double parameterIntervalX=(parameterMaxX-parameterMinX)/numBinsX;
	 double parameterIntervalY=(parameterMaxY-parameterMinY)/numBinsY;

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

void Histogram2DWindow::onSaveDataClicked()
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
	headingX=axisNameFromParameter(parameterX);
	headingY=axisNameFromParameter(parameterY);
	double intervalX=(parameterMaxX-parameterMinX)/numBinsX;
	double intervalY=(parameterMaxY-parameterMinY)/numBinsY;
	QTextStream stream( &exportFile );
	//stream<<headingX<<" \t"<<headingY<<endl;
	for (int i=0;i<numBinsX;i++)
	{
		for (int j=0;j<numBinsY;j++)
		{
			double xVal=parameterMinX+i*intervalX;
			double yVal=parameterMinY+j*intervalY;
			//stream<<xVal<<" \t"<<yVal<<" \t"<<values[i+j*numBinsX]<<endl;
			stream<<values[i+j*numBinsX]<<" ";
			//stream<<values[i+j*numBinsX]<<" ";
		}
		stream<<endl;
	}

	stream.flush();
	exportFile.close();
}

void Histogram2DWindow::updateSettings()
{
	ui.qwtPlotHistogram2D->setLogScale(logScale);	
	ui.actionLogarithmicColourScale->setChecked(logScale);
	ui.actionBilinearSmoothing->setChecked(smoothing);
	ui.qwtPlotHistogram2D->setXRangeParameter(parameterMinX, parameterMaxX);
	ui.qwtPlotHistogram2D->setYRange(parameterMinY, parameterMaxY);
	updateTitleAndAxis();
	recreateHistogram();
}

QDataStream &operator<<(QDataStream &out, const Histogram2DWindow &obj)
{
	out<<static_cast<const PlotWindow&>(obj);
	out<<UI_SAVE_VERSION;	
	quint32 parmX=obj.parameterX, parmY=obj.parameterY;
	out<<parmX<<parmY<<obj.numBinsX<<obj.numBinsY;
	out<<obj.parameterMinX<<obj.parameterMaxX<<obj.parameterMinY<<obj.parameterMaxY;
	out<<obj.logScale<<obj.smoothing<<obj.displayRegions;

	return out;	
}

QDataStream &operator>>(QDataStream &in, Histogram2DWindow &obj)
{
	in>>static_cast<PlotWindow&>(obj);
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	quint32 parmX, parmY;
	in>>*((quint32*)&obj.parameterX);
	in>>*((quint32*)&obj.parameterY);
	in>>obj.numBinsX>>obj.numBinsY;
	in>>obj.parameterMinX>>obj.parameterMaxX>>obj.parameterMinY>>obj.parameterMaxY;
	in>>obj.logScale>>obj.smoothing>>obj.displayRegions;
	
	obj.updateSettings();
	return in;
}