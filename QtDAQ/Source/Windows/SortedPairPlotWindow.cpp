#include "Windows/SortedPairPlotWindow.h"

SortedPairPlotWindow::SortedPairPlotWindow(QWidget * parent, int s_chPrimary, int s_chSecondary, HistogramParameter s_parameter, int s_refreshDelay, bool s_autoscale): PlotWindow(parent, s_chPrimary), 
	chSecondary(s_chSecondary), parameter(s_parameter), refreshDelay(s_refreshDelay), autoscale(s_autoscale)
{
	ui.setupUi(this);

	plot=ui.qwtPlotSortedPair;
	ui.qwtPlotSortedPair->setMinimumHeight(200);
	ui.qwtPlotSortedPair->setMinimumWidth(400);
	ui.qwtPlotSortedPair->setAxisScale(QwtPlot::yLeft, 0, 100, 10);
	ui.qwtPlotSortedPair->setAxisScale(QwtPlot::xBottom, 0, 100, 10);
	updateTitleAndAxis();	
}

void SortedPairPlotWindow::onNewEventStatistics(QVector<EventStatistics*>* events)
{	
	 if (!events)
        return;
	 
	 //reserve mem
	 xVals.reserve(xVals.size()+events->size());
	 yVals.reserve(yVals.size()+events->size());
	 //TODO: cuts!
    for (QVector<EventStatistics*>::iterator it=events->begin();it!=events->end();it++)
	{
		EventStatistics* eventStats=*it;
		SampleStatistics statsPrimary=eventStats->channelStatistics[chPrimary];
		SampleStatistics statsSecondary=eventStats->channelStatistics[chSecondary];		

		int channelNumPrimary=statsPrimary.channelNumber;
		int channelNumSecondary=statsSecondary.channelNumber;
		if (channelNumPrimary!=chPrimary ||channelNumSecondary!=chSecondary || !isInActiveRegion(eventStats))
			continue;

		xVals<<valueFromStatsAndParameter(statsPrimary, parameter);
		yVals<<valueFromStatsAndParameter(statsSecondary, parameter);
	}
	qSort(xVals);
	qSort(yVals);
	ui.qwtPlotSortedPair->setSamples(xVals, yVals);
	ui.qwtPlotSortedPair->update();
}

SortedPairPlotWindow::~SortedPairPlotWindow()
{

}

void SortedPairPlotWindow::onOptionsClicked()
{
	QDialog* dialog=new QDialog(this);
    uiDialogSortedPairPlotConfig.setupUi(dialog);
	uiDialogSortedPairPlotConfig.comboBoxChannel->setCurrentIndex(chPrimary);
	uiDialogSortedPairPlotConfig.comboBoxChannelSecondary->setCurrentIndex(chSecondary);
	uiDialogSortedPairPlotConfig.comboBoxParameter->setCurrentIndex(parameter);
	uiDialogSortedPairPlotConfig.spinBoxDelay->setValue(refreshDelay);
	uiDialogSortedPairPlotConfig.checkBoxAutoscale->setChecked(autoscale);
    int retDialog=dialog->exec();
    if (retDialog==QDialog::Rejected)
        return;

	bool needsClear=false;
	int newChPrimary=uiDialogSortedPairPlotConfig.comboBoxChannel->currentIndex();
	int newChSecondary=uiDialogSortedPairPlotConfig.comboBoxChannelSecondary->currentIndex();
	if (newChPrimary!=chPrimary || newChSecondary!=chSecondary)
	{
		chPrimary=newChPrimary;
		chSecondary=newChSecondary;
		needsClear=true;
		emit channelChanged(chPrimary, chSecondary);
	}		
	HistogramParameter newParameter=(HistogramParameter)uiDialogSortedPairPlotConfig.comboBoxParameter->currentIndex();
	if (parameter!=newParameter)
	{
		parameter=newParameter;
		needsClear=true;		
	}
	if (needsClear)
	{
		updateTitleAndAxis();
		clearValues();
	}

	refreshDelay=uiDialogSortedPairPlotConfig.spinBoxDelay->value();
	autoscale=uiDialogSortedPairPlotConfig.checkBoxAutoscale->isChecked();	
}

void SortedPairPlotWindow::updateTitleAndAxis()
{
	QString windowTitle;
	if (!name.isEmpty())
		windowTitle+=name+": ";
	windowTitle+="Ch "+QString::number(chPrimary)+", "+QString::number(chSecondary);	
	windowTitle+=" Sorted Pairs (";
	QString axisName=axisNameFromParameter(parameter);	

	windowTitle+=axisName;
	windowTitle+=")";	
	setWindowTitle(windowTitle);
	ui.qwtPlotSortedPair->setAxisTitle(QwtPlot::xBottom, axisName+ " (Ch "+QString::number(chPrimary)+")");
	ui.qwtPlotSortedPair->setAxisTitle(QwtPlot::yLeft, axisName+ " (Ch "+QString::number(chSecondary)+")");
}

void SortedPairPlotWindow::clearValues()
{
	xVals.clear();
	yVals.clear();	
	ui.qwtPlotSortedPair->replot();
}

void SortedPairPlotWindow::timerUpdate()
{
	
}

void SortedPairPlotWindow::onSaveDataClicked()
{

}

