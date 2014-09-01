#pragma once

#include "DRS4Acquisition.h"
#include "ui_windowSignalPlot.h"
#include "ui_dialogSignalPlotConfig.h"
#include "Windows/PlotWindow.h"
#include "Plots/SignalPlot.h"

#define UI_SAVE_VERSION_SIGPLOT ((quint32)(0x04))

class SignalPlotWindow : public PlotWindow
{
	Q_OBJECT
public:
	SignalPlotWindow (QWidget * parent = 0, int s_chPrimary=0, int s_chSecondary=-1, int s_refreshDelay=100, bool s_autoscale=false, bool s_averageWaveform=false);
	void setProjectorMode(bool s_projectorMode);
	~SignalPlotWindow ();

public slots:
	void onNewEventSample(EventSampleData* sample);
	void onOptionsClicked();
	void onDisplayAverageClicked(bool checked);
	void clearValues();
	void onSaveDataClicked();
	void timerUpdate();
	void onShowToFToggled(bool);
	void onShowCIToggled(bool);
	void onAutoscaleToggled(bool);
	void onAlignSignalsToggled(bool);
	void onLogarithmicScaleToggled(bool);
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
	bool averageWaveform = false;
	bool autoscale = false; 
	bool alignSigs = true;
	bool showToFGates = false;
	bool showCIGates = true;

private:
	friend QDataStream &operator<<(QDataStream &out, const SignalPlotWindow &obj);
	friend QDataStream &operator>>(QDataStream &in, SignalPlotWindow &obj);
};