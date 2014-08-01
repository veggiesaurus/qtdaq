#pragma once

#include "DRS4Acquisition.h"
#include "ui_windowSignalPlot.h"
#include "ui_dialogSignalPlotConfig.h"
#include "Windows/PlotWindow.h"
#include "Plots/SignalPlot.h"

class SignalPlotWindow : public PlotWindow
{
	Q_OBJECT
public:
	SignalPlotWindow (QWidget * parent = 0, int s_chPrimary=0, int s_chSecondary=-1, int s_refreshDelay=100, bool s_autoscale=false, bool s_averageWaveform=false);
	void onNewEventSample(EventSampleData* sample);
	~SignalPlotWindow ();

public slots:
	void onOptionsClicked();
	void onDisplayAverageClicked(bool checked);
	void clearValues();
	void onSaveDataClicked();
	void timerUpdate();
signals:
	void plotClosed();
private:
	void updateTitle();	
	//called after serialization
	void updateSettings();

private:
	Ui::WindowSignalPlot ui;
	Ui::DialogSignalConfig uiDialogSignalConfig;
	Ui::DialogActiveCuts uiDialogActiveCuts;

	int chSecondary;
	int refreshDelay;
	bool averageWaveform;
	bool autoscale;

private:
	friend QDataStream &operator<<(QDataStream &out, const SignalPlotWindow &obj);
	friend QDataStream &operator>>(QDataStream &in, SignalPlotWindow &obj);
};