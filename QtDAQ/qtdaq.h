#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QVector>
#include <QString>
#include <QTime>
#include <QSettings>
#include <QtSerialPort\QSerialPort>
#include <QtSerialPort\QSerialPortInfo>
#include <QInputDialog>

#include "ui_qtdaq.h"
#include "ui_dialogConfig.h"
#include "ui_windowHistogramPlot.h"
#include "ui_window2DHistogramPlot.h"
#include "ui_windowSignalPlot.h"

#include "globals.h"
#include "Cuts.h"

#include "AcquisitionConfig.h"
#include "AnalysisConfig.h"

#include "Dialogs/LinearCutDialog.h"
#include "Dialogs/LinearCutEditDialog.h"
#include "Dialogs/CalibrationDialog.h"
#include "Dialogs/AnalysisConfigDialog.h"
#include "Dialogs/ConfigDialog.h"

#include "Windows/SignalPlotWindow.h"
#include "Windows/HistogramWindow.h"
#include "Windows/Histogram2DWindow.h"
#include "Windows/FoMWindow.h"
#include "Windows/SortedPairPlotWindow.h"
#include "Threads/DRSBinaryReaderThread.h"
#include "Threads/DRSAcquisitionThread.h"
#include "Threads/VxBinaryReaderThread.h"
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
	Vx1761_MODE = 2
};


class QtDAQ : public QMainWindow
{
	Q_OBJECT

public:
	QtDAQ(QWidget *parent = 0);
	~QtDAQ();
	public slots:
	//config slots
	void onDRSObjectChanged(DRS* s_drs, DRSBoard* s_board);
	//thread slots
	void onNewProcessedEvents(QVector<EventStatistics*>* stats);
	void onNewEventSample(EventSampleData* sample);
	void onUiUpdateTimerTimeout();
	void onEventReadingFinished();
	//file slots
	void onReadDRSFileClicked();
	void onReadVx1761FileClicked();
	void onReadStatisticsFileClicked();
	void onReplayCurrentFileClicked();
	//acquisition
	void onInitClicked();
	void onStartClicked();
	void onStopClicked();
	void onResetClicked();
	void onSerialInterfaceClicked();
	//config slots
	void onEditConfigClicked();
	void onLoadConfigClicked();
	void onSaveConfigClicked();
	//options
	void onCalibrateClicked();
	void onEditAnalysisConfigClicked();
	void onSaveAnalysisConfigClicked();
	void onLoadAnalysisConfigClicked();
	void onPauseReadingToggled(bool checked);
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
	void onCascadeClicked();
	void onTileClicked();
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
	void addSortedPairPlotFromSave(int chPrimary, int chSecondary, HistogramParameter parameter, QVector<Condition> conditions, QByteArray geometry);
	bool initSerial(QString portName);
	void setupPlotWindow(PlotWindow*  plotWindow, bool appendConditions = true);
	void clearAllPlots();
private:
	Ui::QtDAQClass ui;
	Ui::DialogConfig uiDialogConfig;
	Ui::DialogAnalysisConfig uiDialogAnalysisConfig;
	AcquisitionConfig* config;
	AnalysisConfig* analysisConfig;
	EnergyCalibration calibrationValues[NUM_DIGITIZER_CHANNELS];
	QVector<SignalPlotWindow*> signalPlots;
	QVector<HistogramWindow*> histograms;
	QVector<Histogram2DWindow*> histograms2D;
	QVector<FoMWindow*> fomPlots;
	QVector<SortedPairPlotWindow*> sortedPairPlots;
	QVector<LinearCut> cuts;
	QVector<PolygonalCut> polygonalCuts;
	DRSBinaryReaderThread* drsReaderThread;
	VxBinaryReaderThread* vxReaderThread;
	VxProcessThread* vxProcessThread;
	DRSAcquisitionThread* drsAcquisitionThread;
	int numEventsProcessed, prevNumEventsProcessed;
	int numUITimerTimeouts;
	bool finishedReading;
	float prevAcquisitionTime;
	QTimer* uiUpdateTimer;
	QTime* acquisitionTime;
	DRS* drs;
	DRSBoard* board;
	QString rawFilename;
	QString filename;
	DAQStatus status;
	QSettings settings;
	DataMode dataMode;

	//Serial interface
	QSerialPort* serial;
	QString serialString;
	float currentTemp;

	//buffers and mutexes
	//raw 
	QMutex* rawBuffer1Mutex;
	EventVx* rawBuffer1;
	QMutex* rawBuffer2Mutex;
	EventVx* rawBuffer2;
	//processed events
	QMutex* procBuffer1Mutex;
	EventStatistics* procBuffer1;
	QMutex* procBuffer2Mutex;
	EventStatistics* procBuffer2;
};