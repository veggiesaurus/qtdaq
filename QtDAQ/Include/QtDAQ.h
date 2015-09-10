#pragma once
#include <QtWidgets/QMainWindow>
#include <QVector>
#include <QTime>
#include <QSettings>
#include <QtSerialPort/QSerialPort>
#ifdef WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#endif
#include "ui_qtdaq.h"
#include "ui_dialogAcquisitionConfig.h"

#include "Cuts.h"

#include "DRSAcquisitionConfig.h"
#include "AnalysisConfig.h"

#include "Dialogs/LinearCutDialog.h"
#include "Dialogs/LinearCutEditDialog.h"
#include "Dialogs/CalibrationDialog.h"
#include "Dialogs/AnalysisConfigDialog.h"
#include "Dialogs/AcquisitionConfigDialog.h"
#include "Dialogs/VxAcquisitionConfigDialog.h"

#include "Windows/SignalPlotWindow.h"
#include "Windows/HistogramWindow.h"
#include "Windows/Histogram2DWindow.h"
#include "Windows/FoMWindow.h"
#include "Windows/SortedPairPlotWindow.h"

#include "Threads/DRSBinaryReaderThread.h"
#include "Threads/DRSAcquisitionThread.h"
#include "Threads/VxBinaryReaderThread.h"
#include "Threads/VxAcquisitionThread.h"
#include "Threads/VxProcessThread.h"


enum DAQStatus
{
	DAQ_CONFIGURED = 1 << 0,
	DAQ_INIT = 1 << 1,
	DAQ_ACQUIRING = 1 << 2,
	DAC_READING = 1 << 3,
	DAQ_FINISHED = 1 << 4,
	DAQ_ERROR = 1 << 5
};

enum DataMode
{
	NONE = 0,
	DRS_MODE = 1,
	Vx_MODE = 2
};

//
#define NUM_UI_PAGES 4

class QtDAQ : public QMainWindow
{
	Q_OBJECT

public:
	QtDAQ(QWidget *parent = 0);
	~QtDAQ();
	public slots:
	//thread slots
	void onNewProcessedEvents(QVector<EventStatistics*>* stats);
	void onNewEventSample(EventSampleData* sample);
	void onUiUpdateTimerTimeout();
	void onEventReadingFinished();
	void onFilePercentUpdated(float filePercent);
	//file slots
	void onReadDRSFileClicked();
	void onReadVxFileClicked();
	void onReadStatisticsFileClicked();
	void onReplayCurrentFileClicked();
    void onSetOutputFileVxClicked();
	//DRS acquisition
	void onDRSInitClicked();
	void onDRSStartClicked();
	void onDRSStopClicked();
	void onDRSResetClicked();
	void onDRSSoftTriggerClicked();
	void onDRSAutoTriggerToggled(bool);
	void onSerialInterfaceClicked();
	//DRS acquisition config slots
	void onDRSEditConfigFileClicked();
	void onDRSLoadConfigFileClicked();
	void onDRSSaveConfigFileClicked();
	void onDRSObjectChanged(DRS* s_drs, DRSBoard* s_board);
	//Vx acquisition
	void onVxStartClicked();
	void onVxResetClicked();
	void onVxStopClicked();
	void onVxInitClicked();
	void onVxSoftTriggerClicked();
	//Vx acquisition config slots
	void onVxLoadConfigFileClicked();
	void onVxEditConfigFileClicked();
	void onVxSaveConfigFileClicked();	
	//options
	void onCalibrateClicked();
	void onEditAnalysisConfigClicked();
	void onSaveAnalysisConfigClicked();
	void onLoadAnalysisConfigClicked();
	void onPauseReadingToggled(bool checked);
	void onLoadTemperatureLogClicked();
	//cuts
	void onAddLinearCutClicked();
	void onEditLinearCutClicked();
	void onRemoveLinearCutClicked();
	void onPolygonalCutCreated(PolygonalCut cut);
	void onLinearCutCreated(LinearCut cut);
	void onCutRemoved(int cutIndex, bool isPolygonal);

	//view
	void onAddHistogramClicked();
	void onAddHistogram2DClicked();
	void onAddSortedPairPlotClicked();
	void onAddSignalPlotClicked();
	void onAddFigureOfMeritClicked();
	void onSaveUIClicked();
	void onRestoreUIClicked();
	void onRestoreUILegacyClicked();
	void onCascadeClicked();
	void onTileClicked();
	void onProjectorModeToggled(bool);
	//tabs
	void onMoveToPage1Clicked();
	void onMoveToPage2Clicked();
	void onMoveToPage3Clicked();
	void onMoveToPage4Clicked();
	void onSwitchPage1();
	void onSwitchPage2();
	void onSwitchPage3();
	void onSwitchPage4();
	void onRenamePageClicked();
	//inter-window
	void onHistogramClosed();
	void onHistogram2DClosed();
	void onFoMPlotClosed();
	void onSignalPlotClosed();
	void onSortedPairPlotClosed();
	void onHistogramChannelChanged(int);
	void onHistogram2DChannelChanged(int);
	void onFoMPlotChannelChanged(int);

	//duplication
	void onHistogramDuplication(HistogramWindow*);
	void onHistogram2DDuplication(Histogram2DWindow*);
	void onFoMPlotDuplication();
	void onSignalPlotDuplication();
	//exit
	void onExitClicked();
	//temperature sensor
	void serialReadData();
protected:
	void closeEvent(QCloseEvent *event);

signals:
	void newEvents(QVector<EventStatistics*>* stats);
	void calibrationComplete(int);
	void calibrationChanged(int, EnergyCalibration);
	void cutAdded(LinearCut&, bool);
	void polygonalCutAdded(PolygonalCut&, bool);
	void cutRemoved(int index, bool isPolygonal);
	void polygonalCutRemoved(PolygonalCut&);
	void temperatureUpdated(float);
	void resumeProcessing();
private:
	void readVxFile(QString filename, bool compressedInput);
	void addSortedPairPlotFromSave(int chPrimary, int chSecondary, HistogramParameter parameter, QVector<Condition> conditions, QByteArray geometry);
	void restoreUI(bool legacy = false);
	bool initSerial(QString portName);
	void setupPlotWindow(PlotWindow*  plotWindow, int page=-1, bool appendConditions = true);
	void moveCurrentWindowToPage(int page);
	void switchToPage(int page);
	void clearAllPlots();
private:
	Ui::QtDAQClass ui;
	QMdiArea* mdiAreas[4];
	Ui::DialogAcquisitionConfig uiDialogAcquisitionConfig;
	Ui::DialogAnalysisConfig uiDialogAnalysisConfig;
	DRSAcquisitionConfig* acquisitionConfig;
	QString vxAcquisitionConfigString;
	VxAcquisitionConfig* vxConfig;
	AnalysisConfig* analysisConfig;
	EnergyCalibration calibrationValues[NUM_DIGITIZER_CHANNELS];
	QVector<SignalPlotWindow*> signalPlots;
	QVector<HistogramWindow*> histograms;
	QVector<Histogram2DWindow*> histograms2D;
	QVector<FoMWindow*> fomPlots;
	QVector<SortedPairPlotWindow*> sortedPairPlots;
	QVector<LinearCut> cuts;
	QVector<PolygonalCut> polygonalCuts;
	DRSBinaryReaderThread* drsReaderThread = nullptr;
	VxBinaryReaderThread* vxReaderThread = nullptr;
	VxAcquisitionThread* vxAcquisitionThread = nullptr;
	VxProcessThread* vxProcessThread = nullptr;
	DRSAcquisitionThread* drsAcquisitionThread = nullptr;
	int numEventsProcessed = 0, prevNumEventsProcessed = 0;
	int numUITimerTimeouts = 0;
	bool finishedReading;
	float prevAcquisitionTime;
	QTimer uiUpdateTimer;

	QTimer autoTrigTimer;
	QTime acquisitionTime;
	DRS* drs = nullptr;
	DRSBoard* board = nullptr;
	QString rawFilename;
	QString filename;
	DAQStatus status;
	QSettings settings;
	DataMode dataMode;

	//Serial interface
	QSerialPort* serial = nullptr;
	QString serialString;
	float currentTemp;

	//buffers and mutexes
	//raw 
	QMutex* rawBuffer1Mutex;
	EventVx* rawBuffer1;
	QMutex* rawBuffer2Mutex;
	EventVx* rawBuffer2;
	int bufferLength;
	//processed events

	//run tracking
	int runIndex = 0;

	//projector mode
	bool projectorMode = true;

	//progress in taskbar
#ifdef WIN32
	QWinTaskbarProgress* progressTaskbar = nullptr;
#endif
	float filePercent = 0;

	//temperature
	QString temperatureLogFilename;

};
