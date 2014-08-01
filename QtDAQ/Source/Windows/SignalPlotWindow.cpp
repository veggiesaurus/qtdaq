#include "Windows/SignalPlotWindow.h"

SignalPlotWindow::SignalPlotWindow(QWidget * parent, int s_chPrimary, int s_chSecondary, int s_refreshDelay, bool s_autoscale, bool s_averageWaveform): PlotWindow(parent, s_chPrimary),
	chSecondary(s_chSecondary),refreshDelay(s_refreshDelay),autoscale(s_autoscale), averageWaveform(s_averageWaveform)
{
	ui.setupUi(this);
	
	plot=ui.qwtPlotSignal;
	ui.qwtPlotSignal->setMinimumHeight(200);
	ui.qwtPlotSignal->setMinimumWidth(400);
	ui.qwtPlotSignal->setCanvasBackground(QBrush(Qt::white));
	ui.qwtPlotSignal->setAxisScale(QwtPlot::yLeft, 0, 1024, 1000);
	ui.qwtPlotSignal->setAxisScale(QwtPlot::xBottom, 0, 520, 100);
	ui.qwtPlotSignal->setAxisTitle(QwtPlot::xBottom, "t (nS)");
	ui.qwtPlotSignal->setAxisTitle(QwtPlot::yLeft, "ADC Count");
	updateTitle();	
}

void SignalPlotWindow::onNewEventSample(EventSampleData* sample)
{					
	int channelNum=sample->stats.channelStatistics[chPrimary].channelNumber;
	if (channelNum!=chPrimary || !isInActiveRegion(&sample->stats))
		return;
	
	if ((chPrimary>=0 && sample->fValues[chPrimary]) ||(chSecondary>=0 && sample->fValues[chSecondary]))
	{
		float sampleTime=1000.0/sample->MSPS;
		float offset=sample->cfdTime-0.1f*sampleTime*sample->numSamples;
		ui.qwtPlotSignal->setData(sample->tValues, chPrimary>=0?sample->fValues[chPrimary]:NULL, chSecondary>=0?sample->fValues[chSecondary]:NULL, sample->numSamples, sampleTime, offset);	
		ui.qwtPlotSignal->setBaseline(sample->baseline);
		ui.qwtPlotSignal->setGates(sample->indexStart, sample->indexShortEnd, sample->indexLongEnd, sample->indexOfTOFStartPulse, sample->indexOfTOFEndPulse);
		ui.qwtPlotSignal->replot();
	}
}

SignalPlotWindow::~SignalPlotWindow()
{

}

void SignalPlotWindow::onOptionsClicked()
{
	QDialog* dialog=new QDialog(this);
    uiDialogSignalConfig.setupUi(dialog);
	uiDialogSignalConfig.comboBoxChannel->setCurrentIndex(chPrimary);
	uiDialogSignalConfig.comboBoxChannelSecondary->setCurrentIndex(chSecondary+1);
	uiDialogSignalConfig.spinBoxDelay->setValue(refreshDelay);
	uiDialogSignalConfig.checkBoxAutoscale->setChecked(autoscale);
    int retDialog=dialog->exec();
    if (retDialog==QDialog::Rejected)
        return;

	chPrimary=uiDialogSignalConfig.comboBoxChannel->currentIndex();
	//the -1 is needed because the first entry in the secondary box is N/A
	chSecondary=uiDialogSignalConfig.comboBoxChannelSecondary->currentIndex()-1;
	refreshDelay=uiDialogSignalConfig.spinBoxDelay->value();
	autoscale=uiDialogSignalConfig.checkBoxAutoscale->isChecked();	
	updateTitle();
}

void SignalPlotWindow::onDisplayAverageClicked(bool checked)
{
	averageWaveform=checked;
	ui.qwtPlotSignal->setDisplayAverage(averageWaveform);
}

void SignalPlotWindow::updateTitle()
{
	QString windowTitle;
	if (!name.isEmpty())
		windowTitle+=name+": ";
	windowTitle=+"Ch "+QString::number(chPrimary);
	if (chSecondary>=0)
		windowTitle+=", "+QString::number(chSecondary)+" ";
	windowTitle+=" Signal";\
	setWindowTitle(windowTitle);
}

void SignalPlotWindow::clearValues()
{
	
}

void SignalPlotWindow::timerUpdate()
{

}

void SignalPlotWindow::onSaveDataClicked()
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

	int numEntries=ui.qwtPlotSignal->numSamples;
	if (numEntries && ui.qwtPlotSignal->tempT && (ui.qwtPlotSignal->tempV || ui.qwtPlotSignal->tempDV))
	{	
		QTextStream stream( &exportFile );
		stream<<"t (ns)"<<" \t"<<((ui.qwtPlotSignal->tempV)?"V (mV) \t":"")<<((ui.qwtPlotSignal->tempDV)?"V2 (mV)":"")<<endl;
		for (int i=0;i<numEntries;i++)
		{
			stream<<ui.qwtPlotSignal->tempT[i];
			if (ui.qwtPlotSignal->tempV)			
				stream<<" \t \t "<<(ui.qwtPlotSignal->displayAverage?ui.qwtPlotSignal->vAverage[i]:ui.qwtPlotSignal->tempV[i]);
			if (ui.qwtPlotSignal->tempDV)						
				stream<<" \t"<<ui.qwtPlotSignal->tempDV[i];
			stream<<endl;
		}
		stream.flush();
		exportFile.close();
	}
	return;
}

void SignalPlotWindow::updateSettings()
{	
	updateTitle();
	ui.actionDisplay_average_waveform->setChecked(averageWaveform);
	ui.qwtPlotSignal->setDisplayAverage(averageWaveform);
}

QDataStream &operator<<(QDataStream &out, const SignalPlotWindow &obj)
{
	out<<static_cast<const PlotWindow&>(obj);
	out<<UI_SAVE_VERSION;	
	out<<obj.chSecondary<<obj.refreshDelay<<obj.averageWaveform<<obj.autoscale;
	return out;	
}

QDataStream &operator>>(QDataStream &in, SignalPlotWindow &obj)
{
	in>>static_cast<PlotWindow&>(obj);
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	in>>obj.chSecondary>>obj.refreshDelay>>obj.averageWaveform>>obj.autoscale;
	obj.updateSettings();
	return in;
}