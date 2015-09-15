#include <QShortcut>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QInputDialog>
#ifdef WIN32
#include <QtWinExtras/QWinTaskbarButton>
#endif
#include "ui_windowHistogramPlot.h"
#include "ui_window2DHistogramPlot.h"
#include "ui_windowSignalPlot.h"
#include "globals.h"

#include "QtDAQ.h"


QtDAQ::QtDAQ(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	mdiAreas[0] = ui.mdiArea1;
	mdiAreas[1] = ui.mdiArea2;
	mdiAreas[2] = ui.mdiArea3;
	mdiAreas[3] = ui.mdiArea4;

	showMaximized();
	projectorMode = ui.actionProjectorMode->isChecked();
	onProjectorModeToggled(projectorMode);

	qRegisterMetaType<DRSAcquisitionConfig>("AcquisitionConfig");
	qRegisterMetaTypeStreamOperators<DRSAcquisitionConfig>("AcquisitionConfig");

	//restore previous settings
	QVariant varAcquisition = settings.value("acquisition/previousSettings");
	if (varAcquisition.isValid())
	{
		acquisitionConfig = new DRSAcquisitionConfig();
		*acquisitionConfig = varAcquisition.value<DRSAcquisitionConfig>();
	}
	else
	{
		acquisitionConfig = DRSAcquisitionConfig::DefaultConfig();
	}
	vxAcquisitionConfigString = settings.value("acquisition/vxPreviousSettings").toString();
	
	vxConfig = VxAcquisitionConfig::parseConfigString(vxAcquisitionConfigString);
	ui.actionVxInit->setEnabled(vxConfig);

	analysisConfig = new AnalysisConfig();
	qRegisterMetaType<AnalysisConfig>("AnalysisConfig");
	qRegisterMetaTypeStreamOperators<AnalysisConfig>("AnalysisConfig");

	//restore previous settings
	QVariant varAnalysis = settings.value("analysis/previousSettings");
	if (varAnalysis.isValid())
		*analysisConfig = varAnalysis.value<AnalysisConfig>();


	//buffers and mutexes
	//raw
	bufferLength = 1024;
	rawBuffer1Mutex = 0;
	rawBuffer2Mutex = 0;
	rawBuffer1 = new EventVx[bufferLength];
	rawBuffer2 = new EventVx[bufferLength];
	memset(rawBuffer1, 0, sizeof(EventVx)*bufferLength);
	memset(rawBuffer2, 0, sizeof(EventVx)*bufferLength);


	drsReaderThread = new DRSBinaryReaderThread(this);
	connect(drsReaderThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
	connect(drsReaderThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
	connect(drsReaderThread, SIGNAL(eventReadingFinished()), this, SLOT(onEventReadingFinished()));
	connect(this, SIGNAL(temperatureUpdated(float)), drsReaderThread, SLOT(onTemperatureUpdated(float)), Qt::QueuedConnection);

	drsAcquisitionThread = new DRSAcquisitionThread(this);
	connect(drsAcquisitionThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
	connect(drsAcquisitionThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
	connect(drsAcquisitionThread, SIGNAL(eventAcquisitionFinished()), this, SLOT(onEventReadingFinished()));
	connect(this, SIGNAL(temperatureUpdated(float)), drsReaderThread, SLOT(onTemperatureUpdated(float)), Qt::QueuedConnection);

	//restore previous temperature sensor (serial) settings
	QVariant varSerialPort = settings.value("temperature/serialPort");
	if (varSerialPort.isValid())
	{
		QString serialPort = varSerialPort.value<QString>();
		initSerial(serialPort);
	}

	//initially uncalibrated
	memset(calibrationValues, 0, sizeof(EnergyCalibration)*NUM_DIGITIZER_CHANNELS);
	ui.mdiArea1->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.mdiArea1->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	new QShortcut(Qt::Key_1, this, SLOT(onSwitchPage1()));
	new QShortcut(Qt::Key_2, this, SLOT(onSwitchPage2()));
	new QShortcut(Qt::Key_3, this, SLOT(onSwitchPage3()));
	new QShortcut(Qt::Key_4, this, SLOT(onSwitchPage4()));

	//update timer for ui
	uiUpdateTimer.setInterval(1000);	
	connect(&uiUpdateTimer, &QTimer::timeout, this, &QtDAQ::onUiUpdateTimerTimeout);

	//auto trigger: 17ms: ~ 60 Hz
	autoTrigTimer.setInterval(17);
	connect(&autoTrigTimer, &QTimer::timeout, this, &QtDAQ::onDRSSoftTriggerClicked);	

#ifdef WIN32
	//progress taskbar
	QWinTaskbarButton *button = new QWinTaskbarButton(this);	
	button->setWindow(windowHandle());	
	progressTaskbar = button->progress();
	progressTaskbar->setVisible(true);
	progressTaskbar->setValue(0);
#endif
}

QtDAQ::~QtDAQ()
{
} 

void QtDAQ::onUiUpdateTimerTimeout()
{
	if (!rawFilename.isEmpty())
	{
		setWindowTitle("QtDAQ - " + rawFilename);
	}
	if (statusBar())
	{
		if (finishedReading)
		{
			double rate = ((double)max(0, numEventsProcessed)) / prevAcquisitionTime*1000.0;
			statusBar()->showMessage("Finished processing " + QString::number(numEventsProcessed) + " events from file @ " + QString::number(rate) + " events/s");
		}
		else
		{
			int timeMillis = uiUpdateTimer.interval();            
			double rate = ((double)max(0, numEventsProcessed - prevNumEventsProcessed)) / timeMillis*1000.0;
			prevNumEventsProcessed = numEventsProcessed;
            numUITimerTimeouts++;
            double averageRate = (double)(numEventsProcessed)/ (numUITimerTimeouts*timeMillis)*1000.0;
            statusBar()->showMessage(QString("Processing data: %1 events processed @ %2 events/s [average: %3 events/s] (%4%)").arg(QString::number(numEventsProcessed), QString::number(rate), QString::number(averageRate), QString::number(filePercent*100)));
		}
	}

}

void QtDAQ::onEventReadingFinished()
{
	prevAcquisitionTime = acquisitionTime.elapsed();

	finishedReading = true;

}

void QtDAQ::onDRSObjectChanged(DRS* s_drs, DRSBoard* s_board)
{
	if (s_board != NULL)
	{
		drs = s_drs;
		board = s_board;		
		ui.actionDRSInit->setEnabled(true);
	}
	else
	{
		ui.actionDRSInit->setEnabled(false);
		ui.actionDRSStart->setEnabled(false);
		ui.actionDRSStop->setEnabled(false);
		ui.actionDRSReset->setEnabled(false);
	}
}

void QtDAQ::onVxInitClicked()
{
	if (rawBuffer1Mutex && !finishedReading)
	{
		rawBuffer1Mutex->lock();
		rawBuffer2Mutex->lock();
	}

	if (vxAcquisitionThread)
	{
		vxAcquisitionThread->stopAcquisition(true);
		vxAcquisitionThread->disconnect();
		vxAcquisitionThread->exit();
		vxAcquisitionThread->wait(500);
		vxAcquisitionThread->terminate();
		SAFE_DELETE(vxAcquisitionThread);
	}

	if (vxProcessThread)
	{
		vxProcessThread->disconnect();
		vxProcessThread->exit();
		vxProcessThread->wait(100);
		vxProcessThread->terminate();
		SAFE_DELETE(vxProcessThread);
	}

	SAFE_DELETE(rawBuffer1Mutex);
	SAFE_DELETE(rawBuffer2Mutex);
	rawBuffer1Mutex = new QMutex();
	rawBuffer2Mutex = new QMutex();
	if (rawBuffer1)
	{
		for (size_t i = 0; i < bufferLength; i++)
			freeVxEvent(rawBuffer1[i]);
	}
	if (rawBuffer2)
	{
		for (size_t i = 0; i < bufferLength; i++)
			freeVxEvent(rawBuffer2[i]);
	}

	SAFE_DELETE_ARRAY(rawBuffer1);
	SAFE_DELETE_ARRAY(rawBuffer2);
	//for acq, small buffer length needed
    bufferLength = 256;
	rawBuffer1 = new EventVx[bufferLength];
	rawBuffer2 = new EventVx[bufferLength];
	memset(rawBuffer1, 0, sizeof(EventVx)*bufferLength);
	memset(rawBuffer2, 0, sizeof(EventVx)*bufferLength);

	vxProcessThread = new VxProcessThread(rawBuffer1Mutex, rawBuffer2Mutex, rawBuffer1, rawBuffer2, bufferLength, this);
	connect(this, SIGNAL(resumeProcessing()), vxProcessThread, SLOT(onResumeProcessing()), Qt::QueuedConnection);
	connect(vxProcessThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
	connect(vxProcessThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
	vxProcessThread->initVxProcessThread(analysisConfig);
	//if (!temperatureLogFilename.isEmpty())
	//		vxProcessThread->loadTemperatureLog(temperatureLogFilename);


	vxAcquisitionThread = new VxAcquisitionThread(rawBuffer1Mutex, rawBuffer2Mutex, rawBuffer1, rawBuffer2, bufferLength, this);

	runIndex++;
	CAENErrorCode errorCode = vxAcquisitionThread->initVxAcquisitionThread(vxConfig, runIndex);
	if (errorCode == ERR_NONE)
	{
		vxAcquisitionThread->setPaused(true);
		vxAcquisitionThread->start();
		vxProcessThread->start();
		ui.actionVxStart->setEnabled(true);
        ui.actionSetOutputFileVx->setEnabled(true);
		if (statusBar())
			statusBar()->showMessage("Successfully opened digitizer for acquisition.");
	}
	else
	{
		if (statusBar())
			statusBar()->showMessage("Failed to init digitizer.");
	}
}

void QtDAQ::onVxStartClicked()
{
	uiUpdateTimer.start();
	acquisitionTime.start();
	numEventsProcessed = 0;
	if (vxAcquisitionThread)
		vxAcquisitionThread->setPaused(false);
	ui.actionVxStop->setEnabled(true);
	ui.actionVxReset->setEnabled(true);
    ui.actionVxInit->setEnabled(false);
	ui.actionVxStart->setEnabled(false);
	ui.actionVxSoftTrigger->setEnabled(true);
    ui.actionSetOutputFileVx->setEnabled(false);
}

void QtDAQ::onVxResetClicked()
{

}

void QtDAQ::onVxStopClicked()
{
	if (vxAcquisitionThread)
		vxAcquisitionThread->setPaused(true);
	ui.actionVxStop->setEnabled(false);
	ui.actionVxReset->setEnabled(false);
	ui.actionVxStart->setEnabled(true);
	ui.actionVxSoftTrigger->setEnabled(false);
    ui.actionSetOutputFileVx->setEnabled(true);
}


void QtDAQ::onVxSoftTriggerClicked()
{

}

void QtDAQ::onVxLoadConfigFileClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set config file", "", "Vx config (*.txt);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.restoreState(settings.value("acquisition/vxSaveAcquisitionConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	QString prevFileDir = settings.value("acquisition/vxPrevAcquisitionDir").toString();
	QString prevFile = settings.value("acquisition/vxPrevAcquisitionFile").toString();
	fileDialog.setDirectory(prevFileDir);
	
	if (fileDialog.exec())
	{
		settings.setValue("acquisition/vxSaveAcquisitionConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("acquisition/vxPrevAcquisitionDir", fileInfo.dir().absolutePath());
		settings.setValue("acquisition/vxPrevAcquisitionFile", fileInfo.fileName());

		QFile file(fileName);

		if (!file.open(QIODevice::ReadOnly | QFile::Text))
			return;
		QTextStream stream(&file);
		QString newConfig = stream.readAll();
		file.close();
		
		vxAcquisitionConfigString = newConfig;
		settings.setValue("acquisition/vxPreviousSettings", vxAcquisitionConfigString);

		vxConfig = VxAcquisitionConfig::parseConfigString(vxAcquisitionConfigString);
		ui.actionVxInit->setEnabled(vxConfig);

		if (!vxConfig)
			QMessageBox::critical(this, "Error in config file", "The config file contains errors. Fix the errors in the editor window before continuing");
	}
}

void QtDAQ::onVxEditConfigFileClicked()
{
	auto dialog = new VxAcquisitionConfigDialog(vxAcquisitionConfigString);
	int retDialog = dialog->exec();

	if (retDialog == QDialog::Rejected)
		return;

	vxAcquisitionConfigString = dialog->ui.plainTextEditCode->toPlainText();
	settings.setValue("acquisition/vxPreviousSettings", vxAcquisitionConfigString);

	vxConfig = VxAcquisitionConfig::parseConfigString(vxAcquisitionConfigString);
	ui.actionVxInit->setEnabled(vxConfig);
}

void QtDAQ::onVxSaveConfigFileClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set output file", "", "Vx config (*.txt);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.restoreState(settings.value("acquisition/vxSaveAcquisitionConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);

	QString prevFileDir = settings.value("acquisition/vxPrevAcquisitionDir").toString();
	QString prevFile = settings.value("acquisition/vxPrevAcquisitionFile").toString();
	fileDialog.setDirectory(prevFileDir);
	fileDialog.selectFile(prevFile);

	if (fileDialog.exec())
	{
		settings.setValue("acquisition/vxSaveAcquisitionConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("acquisition/vxPrevAcquisitionDir", fileInfo.dir().absolutePath());
		settings.setValue("acquisition/vxPrevAcquisitionFile", fileInfo.fileName());

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QFile::Text))
			return;
		QTextStream stream(&file);		
		stream << (vxAcquisitionConfigString);
		stream.flush();
		file.close();
	}
}

void QtDAQ::onNewProcessedEvents(QVector<EventStatistics*>* stats)
{
	if (!stats)
		return;

	//select only valid events
	QVector<EventStatistics*>* statsValid = new QVector<EventStatistics*>();
	//statsValid->reserve(stats->size());
	for (auto& i : *stats)
	{
		if (i->runIndex == runIndex)
			statsValid->push_back(i);			
	}

	numEventsProcessed += statsValid->size();

	for (auto& i : histograms)
		i->onNewEventStatistics(statsValid);

	for (auto& i : histograms2D)
		i->onNewEventStatistics(statsValid);

	for (auto& i : fomPlots)
		i->onNewEventStatistics(statsValid);

	for (auto& i : sortedPairPlots)
		i->onNewEventStatistics(statsValid);

	for (auto& i : *stats)
		SAFE_DELETE(i);

	stats->clear();
	statsValid->clear();
	SAFE_DELETE(stats);
	SAFE_DELETE(statsValid);
}

void QtDAQ::onNewEventSample(EventSampleData* sample)
{
	if (!sample)
		return;
	for (QVector<SignalPlotWindow*>::iterator it = signalPlots.begin(); it != signalPlots.end(); it++)
	{
		(*it)->onNewEventSample(sample);
	}
	SAFE_DELETE_ARRAY(sample->tValues);
	for (int ch = 0; ch<NUM_DIGITIZER_CHANNELS; ch++)
	{
		SAFE_DELETE_ARRAY(sample->fValues[ch]);
	}
	SAFE_DELETE(sample);
}

void QtDAQ::onReadDRSFileClicked()
{
	QFileDialog fileDialog(this, "Set raw data input file", "", "Uncompressed data (*.dat);;Compressed data (*.dtz);;All files (*.*)");
	fileDialog.restoreState(settings.value("mainWindow/loadRawStateDRS").toByteArray());
	QString prevFile = settings.value("mainWindow/prevDRSRawDataFile").toString();
	QString prevFileDir = settings.value("mainWindow/prevDRSRawDataDir").toString();
	fileDialog.setDirectory(prevFileDir);
	fileDialog.selectFile(prevFile);
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (fileDialog.exec())
	{
		QString newRawFilename = fileDialog.selectedFiles().first();
		settings.setValue("mainWindow/loadRawStateDRS", fileDialog.saveState());
		QFileInfo fileInfo(newRawFilename);	
		settings.setValue("mainWindow/prevDRSRawDataDir", fileInfo.dir().absolutePath());
		settings.setValue("mainWindow/prevDRSRawDataFile", fileInfo.fileName());

		rawFilename = newRawFilename;
		clearAllPlots();
		bool compressedOutput = rawFilename.endsWith("dtz");

		uiUpdateTimer.stop();
		numUITimerTimeouts = 0;
		numEventsProcessed = 0;
		prevNumEventsProcessed = 0;

		if (drsReaderThread)
		{
			drsReaderThread->stopReading(!finishedReading);
			drsReaderThread->wait(100);
			drsReaderThread->exit();
			drsReaderThread->disconnect();
			delete drsReaderThread;
		}
		drsReaderThread = new DRSBinaryReaderThread(this);
		connect(drsReaderThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
		connect(drsReaderThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
		connect(drsReaderThread, SIGNAL(eventReadingFinished()), this, SLOT(onEventReadingFinished()));

		runIndex++;
		drsReaderThread->initDRSBinaryReaderThread(rawFilename, compressedOutput, runIndex, analysisConfig);
		drsReaderThread->start();
		finishedReading = false;
		uiUpdateTimer.start();
		acquisitionTime.start();
		dataMode = DRS_MODE;
		ui.actionPauseFileReading->setChecked(false);
	}

}

void QtDAQ::onReadVxFileClicked()
{
    QFileDialog fileDialog(this, "Set raw data input file", "", "Compressed data (*.dtz);;Packed data (*.pcz);;LZO Compressed data (*.dlz);;All files (*.*)");
    fileDialog.restoreState(settings.value("mainWindow/loadRawStateVx").toByteArray());
    QString prevFile = settings.value("mainWindow/prevVxRawDataFile").toString();
    QString prevFileDir = settings.value("mainWindow/prevVxRawDataDir").toString();
    fileDialog.setDirectory(prevFileDir);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (fileDialog.exec())
	{
		QString newRawFilename = fileDialog.selectedFiles().first();
		settings.setValue("mainWindow/loadRawStateVx", fileDialog.saveState());
		QFileInfo fileInfo(newRawFilename);
		settings.setValue("mainWindow/prevVxRawDataDir", fileInfo.dir().absolutePath());
		settings.setValue("mainWindow/prevVxRawDataFile", fileInfo.fileName());

        //ask for confirmation on clear if there's an existing file
        if (!rawFilename.isEmpty() && QMessageBox::question(this, "Clear plots", "Clear all plots before reading?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
                clearAllPlots();
		rawFilename = newRawFilename;
        bool isCompressedHeader = rawFilename.endsWith("dtz") || rawFilename.endsWith("pcz");

		uiUpdateTimer.stop();
		numUITimerTimeouts = 0;
		numEventsProcessed = 0;
		prevNumEventsProcessed = 0;

        readVxFile(rawFilename, isCompressedHeader);
				
		finishedReading = false;
		uiUpdateTimer.start();
		acquisitionTime.start();
		dataMode = Vx_MODE;
		ui.actionPauseFileReading->setChecked(false);
	}
    else
        qDebug()<<"No file";

}

void QtDAQ::readVxFile(QString filename, bool isCompressedHeader)
{
	if (rawBuffer1Mutex && !finishedReading)
	{
		rawBuffer1Mutex->lock();
		rawBuffer2Mutex->lock();
	}

	if (vxReaderThread)
	{
		vxReaderThread->stopReading(true);
		vxReaderThread->disconnect();
		vxReaderThread->exit();
		vxReaderThread->wait(100);
		vxReaderThread->terminate();
		SAFE_DELETE(vxReaderThread);
	}

	if (vxProcessThread)
	{
		vxProcessThread->disconnect();
		vxProcessThread->exit();
		vxProcessThread->wait(100);
		vxProcessThread->terminate();
		SAFE_DELETE(vxProcessThread);
	}

	SAFE_DELETE(rawBuffer1Mutex);
	SAFE_DELETE(rawBuffer2Mutex);
	rawBuffer1Mutex = new QMutex();
	rawBuffer2Mutex = new QMutex();
	if (rawBuffer1)
	{
		for (size_t i = 0; i < bufferLength; i++)
			freeVxEvent(rawBuffer1[i]);
	}
	if (rawBuffer2)
	{
		for (size_t i = 0; i < bufferLength; i++)
			freeVxEvent(rawBuffer2[i]);
	}

	SAFE_DELETE_ARRAY(rawBuffer1);
	SAFE_DELETE_ARRAY(rawBuffer2);
	//for serial file reading, large buffer is needed
	bufferLength = 1024;
	rawBuffer1 = new EventVx[bufferLength];
	rawBuffer2 = new EventVx[bufferLength];
	memset(rawBuffer1, 0, sizeof(EventVx)*bufferLength);
	memset(rawBuffer2, 0, sizeof(EventVx)*bufferLength);
	//TODO: clean buffers if needed

	vxProcessThread = new VxProcessThread(rawBuffer1Mutex, rawBuffer2Mutex, rawBuffer1, rawBuffer2, bufferLength, this);
	connect(this, SIGNAL(resumeProcessing()), vxProcessThread, SLOT(onResumeProcessing()), Qt::QueuedConnection);
	connect(vxProcessThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
	connect(vxProcessThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
	vxProcessThread->initVxProcessThread(analysisConfig);
	if (!temperatureLogFilename.isEmpty())
		vxProcessThread->loadTemperatureLog(temperatureLogFilename);


	vxReaderThread = new VxBinaryReaderThread(rawBuffer1Mutex, rawBuffer2Mutex, rawBuffer1, rawBuffer2, bufferLength, this);
	connect(vxReaderThread, SIGNAL(eventReadingFinished()), this, SLOT(onEventReadingFinished()));
	connect(vxReaderThread, SIGNAL(filePercentUpdate(float)), this, SLOT(onFilePercentUpdated(float)));

	runIndex++;
    vxReaderThread->initVxBinaryReaderThread(rawFilename, isCompressedHeader, runIndex);
	vxReaderThread->start();
	vxProcessThread->start();

}

void QtDAQ::onReplayCurrentFileClicked()
{
	clearAllPlots();
	if (!rawFilename.isEmpty())
	{
        bool isCompressedHeader = rawFilename.endsWith("dtz") || rawFilename.endsWith("pcz");
		
		ui.actionPauseFileReading->setChecked(false);
		if (dataMode == DRS_MODE)
		{
			drsReaderThread->stopReading(!finishedReading);
			drsReaderThread->wait(100);
			drsReaderThread->exit();
			drsReaderThread->disconnect();
			delete drsReaderThread;
			drsReaderThread = new DRSBinaryReaderThread(this);
			connect(drsReaderThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
			connect(drsReaderThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
			connect(drsReaderThread, SIGNAL(eventReadingFinished()), this, SLOT(onEventReadingFinished()));

			runIndex++;
            drsReaderThread->initDRSBinaryReaderThread(rawFilename, isCompressedHeader, runIndex, analysisConfig);
			drsReaderThread->start();
		}
		else if (dataMode == Vx_MODE)
		{
			if (!finishedReading)
			{
				vxProcessThread->resetTriggerTimerAndV8();
				runIndex++;
				vxReaderThread->rewindFile(runIndex);
				vxReaderThread->setPaused(false);
			}
			else
			{
                readVxFile(rawFilename, isCompressedHeader);
			}
		}
		ui.actionPauseFileReading->setChecked(false);
		uiUpdateTimer.stop();
		numUITimerTimeouts = 0;
		numEventsProcessed = 0;
		prevNumEventsProcessed = 0;
		finishedReading = false;
		uiUpdateTimer.start();
		acquisitionTime.start();
    }
}

void QtDAQ::onSetOutputFileVxClicked()
{
    QFileDialog fileDialog(this, "Set raw data output file", "", "Compressed data (*.dtz);;Packed data (*.pcz);;LZO Compressed data (*.dlz);;All files (*.*)");
    fileDialog.restoreState(settings.value("mainWindow/saveRawStateVx").toByteArray());
    QString prevFile = settings.value("mainWindow/prevVxRawDataFile").toString();
    QString prevFileDir = settings.value("mainWindow/prevVxRawDataDir").toString();
    fileDialog.setDirectory(prevFileDir);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    if (fileDialog.exec())
    {
        QString newRawFilename = fileDialog.selectedFiles().first();
        settings.setValue("mainWindow/saveRawStateVx", fileDialog.saveState());
        QFileInfo fileInfo(newRawFilename);
        settings.setValue("mainWindow/prevVxRawDataDir", fileInfo.dir().absolutePath());
        settings.setValue("mainWindow/prevVxRawDataFile", fileInfo.fileName());

        FileFormat fileFormat = newRawFilename.endsWith("dtz")?GZIP_COMPRESSED:(newRawFilename.endsWith("pcz")?GZIP_COMPRESSED_PACKED:LZO_COMPRESSED);
        if (vxAcquisitionThread)
            vxAcquisitionThread->setFileOutput(newRawFilename, fileFormat);
    }
}

void QtDAQ::onReadStatisticsFileClicked()
{

}


void QtDAQ::onDRSInitClicked()
{
	if (drsAcquisitionThread->isRunning())
		drsAcquisitionThread->reInit(acquisitionConfig, analysisConfig);
	else
	{
		drsAcquisitionThread->initDRSAcquisitionThread(drs, board, acquisitionConfig, analysisConfig);
		//start paused
		drsAcquisitionThread->setPaused(true);
		drsAcquisitionThread->start();
		ui.actionDRSStart->setEnabled(true);
	}
}



void QtDAQ::onDRSStartClicked()
{
	uiUpdateTimer.start();
	acquisitionTime.start();
	numEventsProcessed = 0;
	drsAcquisitionThread->setPaused(false);
	ui.actionDRSStop->setEnabled(true);
	ui.actionDRSReset->setEnabled(true);
	ui.actionDRSStart->setEnabled(false);
	ui.actionDRSSoftTrigger->setEnabled(true);
	ui.actionDRSAutoTrigger->setEnabled(true);
}


void QtDAQ::onDRSStopClicked()
{
	drsAcquisitionThread->setPaused(true);
	ui.actionDRSStart->setEnabled(true);
	ui.actionDRSStop->setEnabled(false);
	ui.actionDRSReset->setEnabled(false);
	ui.actionDRSSoftTrigger->setEnabled(false);
	ui.actionDRSAutoTrigger->setEnabled(false);
}

void QtDAQ::onDRSResetClicked()
{
	clearAllPlots();
	uiUpdateTimer.start();
	acquisitionTime.start();
	numEventsProcessed = 0;
}

void QtDAQ::onDRSSoftTriggerClicked()
{	
	if (drs && board)
		board->SoftTrigger();
}

void QtDAQ::onDRSAutoTriggerToggled(bool triggerOn)
{
	if (triggerOn)
		autoTrigTimer.start();
	else
		autoTrigTimer.stop();
}


void QtDAQ::onDRSEditConfigFileClicked()
{
	AcquisitionConfigDialog* dialog = new AcquisitionConfigDialog(acquisitionConfig);
	connect(dialog, SIGNAL(drsObjectChanged(DRS*, DRSBoard*)), this, SLOT(onDRSObjectChanged(DRS*, DRSBoard*)));
	dialog->setDRS(drs);
	int retDialog = dialog->exec();

	if (retDialog == QDialog::Rejected)
		return;
	if (drsAcquisitionThread)
		drsAcquisitionThread->lockConfig();
	dialog->updateConfig();
	if (drsAcquisitionThread)
		drsAcquisitionThread->unlockConfig();
	settings.setValue("acquisition/previousSettings", QVariant::fromValue(*acquisitionConfig));
}

void QtDAQ::onEditAnalysisConfigClicked()
{
	AnalysisConfigDialog* dialog = new AnalysisConfigDialog(analysisConfig);
	int retDialog = dialog->exec();

	if (retDialog == QDialog::Rejected)
		return;
	dialog->updateConfig();
	if (vxProcessThread)
		vxProcessThread->onAnalysisConfigChanged();
	settings.setValue("analysis/previousSettings", QVariant::fromValue(*analysisConfig));
    dialog->close();
	SAFE_DELETE(dialog);
}

void QtDAQ::onDRSLoadConfigFileClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set config file", "", "Binary config (*.bcfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.restoreState(settings.value("acquisition/saveConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);

	QString prevFileDir = settings.value("acquisition/prevDRSAcqDir").toString();
	QString prevFile = settings.value("acquisition/prevDRSAcqFile").toString();
	fileDialog.setDirectory(prevFileDir);
	fileDialog.selectFile(prevFile);

	if (fileDialog.exec())
	{
		settings.setValue("acquisition/saveConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("acquisition/prevDRSAcqDir", fileInfo.dir().absolutePath());
		settings.setValue("acquisition/prevDRSAcqFile", fileInfo.fileName());
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
			return;
		QDataStream stream(&file);
		stream >> (*acquisitionConfig);
		file.close();
		
		settings.setValue("acquisition/previousSettings", QVariant::fromValue(*acquisitionConfig));

		file.close();
	}
}

void QtDAQ::onLoadAnalysisConfigClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set config file", "", "Analysis config (*.acfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.restoreState(settings.value("analysis/saveAnalysisConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (!rawFilename.isEmpty())
	{
		QFileInfo fileInfo(rawFilename);
		fileDialog.setDirectory(fileInfo.dir().absolutePath());
		fileDialog.selectFile(fileInfo.fileName() + ".acfg");
	}
	else
	{
		QString prevFileDir = settings.value("analysis/prevAnalysisDir").toString();
		QString prevFile = settings.value("analysis/prevAnalysisFile").toString();
		fileDialog.setDirectory(prevFileDir);
		//fileDialog.selectFile(prevFile);
	}
	if (fileDialog.exec())
	{
		settings.setValue("analysis/saveAnalysisConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("analysis/prevAnalysisDir", fileInfo.dir().absolutePath());
		settings.setValue("analysis/prevAnalysisFile", fileInfo.fileName());

		QFile file(fileName);

		if (!file.open(QIODevice::ReadOnly))
			return;
		QDataStream stream(&file);
		stream >> (*analysisConfig);
		file.close();
		if (vxProcessThread)
			vxProcessThread->onAnalysisConfigChanged();
		settings.setValue("analysis/previousSettings", QVariant::fromValue(*analysisConfig));
	}
}

void QtDAQ::onDRSSaveConfigFileClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set output file", "", "Binary config (*.bcfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.restoreState(settings.value("acquisition/saveConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);

	QString prevFileDir = settings.value("acquisition/prevDRSAcqDir").toString();
	QString prevFile = settings.value("acquisition/prevDRSAcqFile").toString();
	fileDialog.setDirectory(prevFileDir);
	fileDialog.selectFile(prevFile);

	if (fileDialog.exec())
	{
		settings.setValue("acquisition/saveConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("acquisition/prevDRSAcqDir", fileInfo.dir().absolutePath());
		settings.setValue("acquisition/prevDRSAcqFile", fileInfo.fileName());

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly))
			return;
		QDataStream stream(&file);
		stream << (*acquisitionConfig);
		file.close();
	}
}

void QtDAQ::onSaveAnalysisConfigClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set output file", "", "Analysis config (*.acfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.restoreState(settings.value("analysis/saveAnalysisConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);
	if (!rawFilename.isEmpty())
	{
		QFileInfo fileInfo(rawFilename);
		fileDialog.setDirectory(fileInfo.dir());
		fileDialog.selectFile(fileInfo.fileName() + ".acfg");
	}
	else
	{
		QString prevFileDir = settings.value("analysis/prevAnalysisDir").toString();
		QString prevFile = settings.value("analysis/prevAnalysisFile").toString();
		fileDialog.setDirectory(prevFileDir);
		fileDialog.selectFile(prevFile);
	}
	if (fileDialog.exec())
	{
		settings.setValue("analysis/saveAnalysisConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("analysis/prevAnalysisDir", fileInfo.dir().absolutePath());
		settings.setValue("analysis/prevAnalysisFile", fileInfo.fileName());

		QFile file(fileName);

		if (!file.open(QIODevice::WriteOnly))
			return;
		QDataStream stream(&file);
		stream << (*analysisConfig);
		file.close();
	}
}

void QtDAQ::onSaveUIClicked()
{
	//open a file for output	
	QFileDialog fileDialog(this, "Set output file", "", "UI config (*.uic);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.restoreState(settings.value("ui/saveUIState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);
	if (!rawFilename.isEmpty())
	{
		QFileInfo fileInfo(rawFilename);
		fileDialog.setDirectory(fileInfo.dir());
		fileDialog.selectFile(fileInfo.fileName() + ".uic");
	}
	else
	{
		QString prevFileDir = settings.value("ui/prevUIDir").toString();
		QString prevFile = settings.value("ui/prevUIFile").toString();
		fileDialog.setDirectory(prevFileDir);
		fileDialog.selectFile(prevFile);
	}
	if (fileDialog.exec())
	{
		settings.setValue("ui/saveUIState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("ui/prevUIDir", fileInfo.dir().absolutePath());
		settings.setValue("ui/prevUIFile", fileInfo.fileName());

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly))
			return;

		QDataStream stream(&file);
		//Version info
		stream << (quint32)UI_SAVE_VERSION;		
		stream << saveGeometry();
		
		//Newly introducted in UI Version 0x03: Begin		
		quint32 numPages = NUM_UI_PAGES;
		stream << numPages;
		quint32 activePage = ui.tabWidget->currentIndex();
		stream << activePage;
		for (auto i = 0; i < numPages; i++)
			stream << ui.tabWidget->tabText(i);
		//Newly introducted in UI Version 0x03: End

		//write calibration values
        stream.writeRawData((const char*)&calibrationValues, sizeof(EnergyCalibration)*NUM_DIGITIZER_CHANNELS);

		//write cuts
		stream << cuts;
		stream << polygonalCuts;

		//write windows 
		int numHists = histograms.count();
        stream << numHists;
		for (int i = 0; i<numHists; i++)
			stream << *histograms[i];

		int numHists2D = histograms2D.count();
		stream << numHists2D;
		for (int i = 0; i<numHists2D; i++)
			stream << *histograms2D[i];

		int numFoMs = fomPlots.count();
		stream << numFoMs;
		for (int i = 0; i<numFoMs; i++)
			stream << *fomPlots[i];

		int numSignalPlots = signalPlots.count();
		stream << numSignalPlots;
		for (int i = 0; i<numSignalPlots; i++)
			stream << *signalPlots[i];

		file.close();
	}
}

void QtDAQ::onRestoreUILegacyClicked()
{
	restoreUI(true);
}

void QtDAQ::onRestoreUIClicked()
{
	restoreUI();
}


void QtDAQ::restoreUI(bool legacy)
{ 
	//open a file for config input
	QFileDialog fileDialog(this, "Set input file", "", "UI config (*.uic);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	//fileDialog.restoreState(settings.value("ui/saveUIState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (!rawFilename.isEmpty())
	{ 
		QFileInfo fileInfo(rawFilename);
		fileDialog.setDirectory(fileInfo.dir().absolutePath());
		fileDialog.selectFile(fileInfo.fileName() + ".uic");
	}
	else 
	{
		QString prevFileDir = settings.value("ui/prevUIDir").toString();
		QString prevFile = settings.value("ui/prevUIFile").toString();
		fileDialog.setDirectory(prevFileDir);
		//fileDialog.selectFile(prevFile);
	}

	if (fileDialog.exec())
	{
		qDebug("Test");
		settings.setValue("ui/saveUIState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(fileName);
		settings.setValue("ui/prevUIDir", fileInfo.dir().absolutePath());
		settings.setValue("ui/prevUIFile", fileInfo.fileName());
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(this, "Error restoring UI", "The UI could not be restored. Please select a different file");
			return;
		}

		//clear up old cuts
		cuts.clear();
		polygonalCuts.clear();

		//clear up old UI
		for (int i = 0; i < histograms.size(); i++)
			histograms[i]->parentWidget()->close();
		histograms.clear();

		for (int i = 0; i < histograms2D.size(); i++)
			histograms2D[i]->parentWidget()->close();
		histograms2D.clear();

		for (int i = 0; i < fomPlots.size(); i++)
			fomPlots[i]->parentWidget()->close();
		fomPlots.clear();

		for (int i = 0; i < signalPlots.size(); i++)
			signalPlots[i]->parentWidget()->close();
		signalPlots.clear();

		for (int i = 0; i < sortedPairPlots.size(); i++)
			sortedPairPlots[i]->parentWidget()->close();
		sortedPairPlots.clear();

		QDataStream stream(&file);
		quint32 fileVersion;
		if (!legacy)
		{
			stream >> fileVersion;
		}
		else
			fileVersion = 0x02;
		QByteArray geometryMainWindow;
		stream >> geometryMainWindow;
		restoreGeometry(geometryMainWindow);

		if (!legacy)
		{		
			//Newly introducted in UI Version 0x03: Begin
			if (fileVersion >= 0x03)
			{
				quint32 numPagesDefined;
				stream >> numPagesDefined;
				quint32 activePage;
				stream >> activePage;				
				for (auto i = 0; i < numPagesDefined; i++)
				{
					QString pageName;
					stream >> pageName;
					if (i < ui.tabWidget->count())
						ui.tabWidget->setTabText(i, pageName);
				}
				if (activePage < ui.tabWidget->count())
					ui.tabWidget->setCurrentIndex(activePage);
			}
			//Newly introducted in UI Version 0x03: End
		}
		//read calibration values
		stream.readRawData((char*)&calibrationValues, sizeof(EnergyCalibration)*NUM_DIGITIZER_CHANNELS);

		//read cuts
		stream >> cuts;
		stream >> polygonalCuts;

		//read windows
		int numHists = 0;
		stream >> numHists;
		for (int i = 0; i<numHists; i++)
		{
			HistogramWindow* histogram = new HistogramWindow(ui.mdiArea1);
			stream >> *histogram;
			setupPlotWindow(histogram, histogram->tabIndex, false);
			histogram->onNewCalibration(histogram->chPrimary, calibrationValues[histogram->chPrimary]);
			histograms.push_back(histogram);

			connect(histogram, SIGNAL(plotClosed()), this, SLOT(onHistogramClosed()));
			connect(histogram, SIGNAL(channelChanged(int)), this, SLOT(onHistogramChannelChanged(int)));
			connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram, SLOT(onNewCalibration(int, EnergyCalibration)));
			connect(histogram, SIGNAL(duplicate(HistogramWindow*)), this, SLOT(onHistogramDuplication(HistogramWindow*)));

		}

		int numHists2D = 0;
		stream >> numHists2D;
		for (int i = 0; i<numHists2D; i++)
		{
			Histogram2DWindow* histogram2D = new Histogram2DWindow(ui.mdiArea1);
			stream >> *histogram2D;
			setupPlotWindow(histogram2D, histogram2D->tabIndex, false);
			histogram2D->onNewCalibration(histogram2D->chPrimary, calibrationValues[histogram2D->chPrimary]);
			histograms2D.push_back(histogram2D);

			connect(histogram2D, SIGNAL(plotClosed()), this, SLOT(onHistogram2DClosed()));
			connect(histogram2D, SIGNAL(channelChanged(int)), this, SLOT(on2DHistogramChannelChanged(int)));
			connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram2D, SLOT(onNewCalibration(int, EnergyCalibration)));
			connect(histogram2D, SIGNAL(duplicate(Histogram2DWindow*)), this, SLOT(onHistogram2DDuplication(Histogram2DWindow*)));
		}

		int numFoMs = 0;
		stream >> numFoMs;
		for (int i = 0; i<numFoMs; i++)
		{
			FoMWindow* fomPlot = new FoMWindow(ui.mdiArea1);
			stream >> *fomPlot;
			setupPlotWindow(fomPlot, fomPlot->tabIndex, false);
			fomPlot->onNewCalibration(fomPlot->chPrimary, calibrationValues[fomPlot->chPrimary]);
			fomPlots.push_back(fomPlot);

			connect(fomPlot, SIGNAL(plotClosed()), this, SLOT(onFoMPlotClosed()));
			connect(fomPlot, SIGNAL(channelChanged(int)), this, SLOT(onFoMPlotChannelChanged(int)));
			connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), fomPlot, SLOT(onNewCalibration(int, EnergyCalibration)));
		}

		int numSignalPlots = 0;
		stream >> numSignalPlots;
		for (int i = 0; i<numSignalPlots; i++)
		{
			SignalPlotWindow* signalPlot = new SignalPlotWindow(ui.mdiArea1);
			stream >> *signalPlot;
			setupPlotWindow(signalPlot, signalPlot->tabIndex, false);
			signalPlots.push_back(signalPlot);

			connect(signalPlot, SIGNAL(plotClosed()), this, SLOT(onSignalPlotClosed()));
		}
		file.close();

		//disable multiple restores
		ui.actionRestoreUI->setEnabled(false);

		//workaround for resize bug
		showNormal();
		this->showMaximized();
		showNormal();
		this->showMaximized();

	}
}
void QtDAQ::onPauseReadingToggled(bool checked)
{
	if (vxReaderThread)
		vxReaderThread->setPaused(checked);
	if (drsReaderThread)
		drsReaderThread->setPaused(checked);
}

void QtDAQ::onCascadeClicked()
{
	ui.mdiArea1->cascadeSubWindows();
}

void QtDAQ::onTileClicked()
{
	int page = ui.tabWidget->currentIndex();
	if (page < 0 || page >= NUM_UI_PAGES)
		return;
	//if there is only one subwindow, maximise it
	if (mdiAreas[page]->subWindowList().count() == 1)
	{
		mdiAreas[page]->subWindowList()[0]->showMaximized();
	}
	else
		mdiAreas[page]->tileSubWindows();
	showNormal();
	showMaximized();
}

void QtDAQ::onCalibrateClicked()
{
	CailbrationDialog calibDlg(this);
	if (calibDlg.exec() == QDialog::Accepted)
	{
		bool checkScale = true;
		bool checkOffset = true;
		double scale = calibDlg.ui.lineEditScale->text().toDouble(&checkScale);
		double offset = calibDlg.ui.lineEditOffset->text().toDouble(&checkOffset);
		int channelNumber = calibDlg.ui.spinBoxChannel->value();
		//if there's a problem or scale is too small
		if (!checkScale || !checkOffset || scale <= 0.01)
			return;
		else
		{
			calibrationValues[channelNumber].scale = scale;
			calibrationValues[channelNumber].offset = offset;
			calibrationValues[channelNumber].calibrated = true;
			//TODO: deal with these
			emit calibrationChanged(channelNumber, calibrationValues[channelNumber]);
		}
	}
}

void QtDAQ::onAddLinearCutClicked()
{
	LinearCutDialog cutDlg(this);
	if (cutDlg.exec() == QDialog::Accepted)
	{
		LinearCut cut;
		cut.channel = cutDlg.ui.comboBoxChannel->currentIndex() - 1;
		cut.parameter = (HistogramParameter)cutDlg.ui.comboBoxParameter->currentIndex();
		cut.cutMin = cutDlg.ui.doubleSpinBoxMin->value();
		cut.cutMax = cutDlg.ui.doubleSpinBoxMax->value();
		cut.name = cutDlg.ui.lineEditName->text();
		cuts.push_back(cut);
		emit cutAdded(cut, true);
	}
}

void QtDAQ::onEditLinearCutClicked()
{
	LinearCutEditDialog cutDlg(cuts, false, this);
	if (cutDlg.exec() == QDialog::Accepted && cuts.size())
	{
		int cutNum = cutDlg.ui.comboBoxName->currentIndex();
		cuts[cutNum].channel = cutDlg.ui.comboBoxChannel->currentIndex() - 1;
		cuts[cutNum].parameter = (HistogramParameter)cutDlg.ui.comboBoxParameter->currentIndex();
		cuts[cutNum].cutMin = cutDlg.ui.doubleSpinBoxMin->value();
		cuts[cutNum].cutMax = cutDlg.ui.doubleSpinBoxMax->value();
	}
}


void QtDAQ::onRemoveLinearCutClicked()
{
	LinearCutEditDialog cutDlg(cuts, true, this);
	if (cutDlg.exec() == QDialog::Accepted && cuts.size())
	{
		int cutNum = cutDlg.ui.comboBoxName->currentIndex();
		emit cutRemoved(cutNum, false);
		cuts.remove(cutNum);
		//do something with it
	}
}

void QtDAQ::setupPlotWindow(PlotWindow* plotWindow, int page, bool appendConditions)
{
	//if no argument passed in, then page defaults to -1, in which case, use current tab
	if (page == -1)
		page = ui.tabWidget->currentIndex();
	if (page < 0 || page >= NUM_UI_PAGES)
		return;
	
	mdiAreas[page]->addSubWindow(plotWindow);
	plotWindow->tabIndex = page;
	plotWindow->setCutVectors(&cuts, &polygonalCuts, appendConditions);
	plotWindow->setProjectorMode(projectorMode);
	connect(this, SIGNAL(cutAdded(LinearCut&, bool)), plotWindow, SLOT(onLinearCutAdded(LinearCut&, bool)));
	connect(this, SIGNAL(polygonalCutAdded(PolygonalCut&, bool)), plotWindow, SLOT(onPolygonalCutAdded(PolygonalCut&, bool)));
	connect(this, SIGNAL(cutRemoved(int, bool)), plotWindow, SLOT(onCutRemoved(int, bool)));
	connect(plotWindow, SIGNAL(linearCutCreated(LinearCut)), this, SLOT(onLinearCutCreated(LinearCut)));
	connect(plotWindow, SIGNAL(polygonalCutCreated(PolygonalCut)), this, SLOT(onPolygonalCutCreated(PolygonalCut)));
	connect(plotWindow, SIGNAL(cutRemoved(int, bool)), this, SLOT(onCutRemoved(int, bool)));
	plotWindow->show();
}

void QtDAQ::onAddHistogramClicked()
{
	HistogramWindow* histogram = new HistogramWindow();
	setupPlotWindow(histogram, -1);
	//set to ch0 by default
	histogram->onNewCalibration(0, calibrationValues[0]);

	connect(histogram, SIGNAL(plotClosed()), this, SLOT(onHistogramClosed()));
	connect(histogram, SIGNAL(channelChanged(int)), this, SLOT(onHistogramChannelChanged(int)));
	connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram, SLOT(onNewCalibration(int, EnergyCalibration)));
	connect(histogram, SIGNAL(duplicate(HistogramWindow*)), this, SLOT(onHistogramDuplication(HistogramWindow*)));

	histograms.push_back(histogram);
}

void QtDAQ::onAddHistogram2DClicked()
{
	Histogram2DWindow* histogram2D = new Histogram2DWindow();
	setupPlotWindow(histogram2D);
	histogram2D->onNewCalibration(0, calibrationValues[0]);
	connect(histogram2D, SIGNAL(plotClosed()), this, SLOT(onHistogram2DClosed()));
	connect(histogram2D, SIGNAL(channelChanged(int)), this, SLOT(onHistogram2DChannelChanged(int)));
	connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram2D, SLOT(onNewCalibration(int, EnergyCalibration)));
	connect(histogram2D, SIGNAL(duplicate(Histogram2DWindow*)), this, SLOT(onHistogram2DDuplication(Histogram2DWindow*)));

	histograms2D.push_back(histogram2D);
}

void QtDAQ::onAddFigureOfMeritClicked()
{
	FoMWindow* fomPlot = new FoMWindow();
	setupPlotWindow(fomPlot);
	fomPlot->onNewCalibration(0, calibrationValues[0]);

	connect(fomPlot, SIGNAL(channelChanged(int)), this, SLOT(on2FoMPlotChannelChanged(int)));
	connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), fomPlot, SLOT(onNewCalibration(int, EnergyCalibration)));
	connect(fomPlot, SIGNAL(plotClosed()), this, SLOT(onFoMPlotClosed()));

	fomPlots.push_back(fomPlot);

}



void QtDAQ::onAddSignalPlotClicked()
{
	SignalPlotWindow* signalPlot = new SignalPlotWindow();
	setupPlotWindow(signalPlot);
	signalPlot->resize(400, 200);

	connect(signalPlot, SIGNAL(plotClosed()), this, SLOT(onSignalPlotClosed()));
	signalPlots.push_back(signalPlot);
}


void QtDAQ::onSignalPlotClosed()
{
	signalPlots.remove(signalPlots.indexOf((SignalPlotWindow*)QObject::sender()));
}

void QtDAQ::onHistogramClosed()
{
	histograms.remove(histograms.indexOf((HistogramWindow*)QObject::sender()));
}

void QtDAQ::onHistogramChannelChanged(int channel)
{
	HistogramWindow* senderWindow = (HistogramWindow*)QObject::sender();
	senderWindow->onNewCalibration(channel, calibrationValues[channel]);
}

void QtDAQ::onHistogram2DClosed()
{
	histograms2D.remove(histograms2D.indexOf((Histogram2DWindow*)QObject::sender()));
}

void QtDAQ::onHistogram2DChannelChanged(int channel)
{
	Histogram2DWindow* senderWindow = (Histogram2DWindow*)QObject::sender();
	senderWindow->onNewCalibration(channel, calibrationValues[channel]);
}

void QtDAQ::onFoMPlotClosed()
{
	fomPlots.remove(fomPlots.indexOf((FoMWindow*)QObject::sender()));
}

void QtDAQ::onFoMPlotChannelChanged(int channel)
{
	FoMWindow* senderWindow = (FoMWindow*)QObject::sender();
	senderWindow->onNewCalibration(channel, calibrationValues[channel]);
}

void QtDAQ::onAddSortedPairPlotClicked()
{
	SortedPairPlotWindow* sortedPairPlotWindow = new SortedPairPlotWindow();
	setupPlotWindow(sortedPairPlotWindow);
	connect(sortedPairPlotWindow, SIGNAL(plotClosed()), this, SLOT(onSortedPairPlotClosed()));
	sortedPairPlots.push_back(sortedPairPlotWindow);
}

void QtDAQ::addSortedPairPlotFromSave(int chPrimary, int chSecondary, HistogramParameter parameter, QVector<Condition> conditions, QByteArray geometry)
{
	SortedPairPlotWindow* sortedPairPlotWindow = new SortedPairPlotWindow(ui.mdiArea1, chPrimary, chSecondary, parameter);
	setupPlotWindow(sortedPairPlotWindow);
	sortedPairPlotWindow->conditions = conditions;
	connect(sortedPairPlotWindow, SIGNAL(plotClosed()), this, SLOT(onSortedPairPlotClosed()));
	sortedPairPlotWindow->parentWidget()->restoreGeometry(geometry);
	sortedPairPlots.push_back(sortedPairPlotWindow);
}

void QtDAQ::onSortedPairPlotClosed()
{
	sortedPairPlots.remove(sortedPairPlots.indexOf((SortedPairPlotWindow*)QObject::sender()));

}

void QtDAQ::onPolygonalCutCreated(PolygonalCut cut)
{
	polygonalCuts.push_back(cut);
	emit polygonalCutAdded(cut, true);
}

void QtDAQ::onLinearCutCreated(LinearCut cut)
{
	cuts.push_back(cut);
	emit cutAdded(cut, true);
}

void QtDAQ::onCutRemoved(int cutIndex, bool isPolygonal)
{
	emit cutRemoved(cutIndex, isPolygonal);
	if (isPolygonal)
		polygonalCuts.remove(cutIndex);
	else
		cuts.remove(cutIndex);
}

void QtDAQ::onHistogramDuplication(HistogramWindow* histogram)
{
	HistogramWindow* sender = (HistogramWindow*)QObject::sender();
	setupPlotWindow(histogram, -1, false);
	histogram->onNewCalibration(histogram->chPrimary, calibrationValues[histogram->chPrimary]);
	histograms.push_back(histogram);

	connect(histogram, SIGNAL(plotClosed()), this, SLOT(onHistogramClosed()));
	connect(histogram, SIGNAL(channelChanged(int)), this, SLOT(onHistogramChannelChanged(int)));
	connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram, SLOT(onNewCalibration(int, EnergyCalibration)));
	connect(histogram, SIGNAL(duplicate(HistogramWindow*)), this, SLOT(onHistogramDuplication(HistogramWindow*)));
}

void QtDAQ::onHistogram2DDuplication(Histogram2DWindow* histogram2D)
{
	Histogram2DWindow* sender = (Histogram2DWindow*)QObject::sender();
	setupPlotWindow(histogram2D, -1, false);
	histogram2D->onNewCalibration(histogram2D->chPrimary, calibrationValues[histogram2D->chPrimary]);
	histograms2D.push_back(histogram2D);
	connect(histogram2D, SIGNAL(plotClosed()), this, SLOT(onHistogram2DClosed()));
	connect(histogram2D, SIGNAL(channelChanged(int)), this, SLOT(onHistogram2DChannelChanged(int)));
	connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram2D, SLOT(onNewCalibration(int, EnergyCalibration)));
	connect(histogram2D, SIGNAL(duplicate(Histogram2DWindow*)), this, SLOT(onHistogram2DDuplication(Histogram2DWindow*)));
}

void QtDAQ::onFoMPlotDuplication()
{
	//TODO

}

void QtDAQ::onSignalPlotDuplication()
{
	//TODO

}

void QtDAQ::onExitClicked()
{
	close();
}

void QtDAQ::onSerialInterfaceClicked()
{
	bool ok;    
    auto list = QSerialPortInfo::availablePorts();
	if (list.isEmpty())
	{
		QMessageBox::critical(this, "Port enumeration failed", "Unable to find any available serial ports.");
		return;
	}
	QList<QString> strList;
    for (auto i:list)
        strList.append(i.portName());

    QString selectedPort = QInputDialog::getItem(this, tr("Select serial port"),
		tr("Port:"), strList, 0, false, &ok);
	if (ok && !selectedPort.isEmpty())
	{
		settings.setValue("temperature/serialPort", selectedPort);
		initSerial(selectedPort);
	}

}


bool QtDAQ::initSerial(QString portName)
{
	if (serial)
	{
		serial->close();
		SAFE_DELETE(serial);
	}
	serial = new QSerialPort(this);
	connect(serial, SIGNAL(readyRead()), this, SLOT(serialReadData()));
	serial->setPortName(portName);
	serial->setReadBufferSize(127);
	if (serial->open(QIODevice::ReadWrite))
	{
		serial->setTextModeEnabled(true);
		if (serial->setBaudRate(QSerialPort::Baud9600)
			&& serial->setDataBits(QSerialPort::Data8)
			&& serial->setParity(QSerialPort::NoParity)
			&& serial->setStopBits(QSerialPort::OneStop)
			&& serial->setFlowControl(QSerialPort::NoFlowControl)
			)
		{
			qDebug("Serial port opened");
			if (statusBar())
				statusBar()->showMessage("Successfully opened serial port " + portName + " for temperature readings.");
			return true;
		}
	}
	SAFE_DELETE(serial);
	if (statusBar())
		statusBar()->showMessage("Could not open serial port " + portName + ". Temperature-adjustments of acquired data will be disabled.");
	return false;
}

void QtDAQ::serialReadData()
{

	QByteArray data = serial->readAll();
	serialString.append(data);
	while (serialString.contains('\n'))
	{
		int index = serialString.indexOf('\n');
		QString line = serialString.mid(0, serialString.indexOf('\n'));
		serialString.remove(0, index + 1);
		QString tempString = line.mid(0, line.indexOf(';'));
		bool validTemp = false;
		float temp = tempString.toFloat(&validTemp);
		if (validTemp)
			emit(temperatureUpdated(temp));

	}

}
void QtDAQ::clearAllPlots()
{
    for (auto plot:histograms)
        plot->clearValues();
    for (auto plot:histograms2D)
        plot->clearValues();
    for (auto plot:fomPlots)
        plot->clearValues();
    for (auto plot:sortedPairPlots)
        plot->clearValues();
}

void QtDAQ::onMoveToPage1Clicked(){ moveCurrentWindowToPage(0); }
void QtDAQ::onMoveToPage2Clicked(){ moveCurrentWindowToPage(1); }
void QtDAQ::onMoveToPage3Clicked(){ moveCurrentWindowToPage(2); }
void QtDAQ::onMoveToPage4Clicked(){ moveCurrentWindowToPage(3); }

void QtDAQ::onSwitchPage1(){ switchToPage(0); }
void QtDAQ::onSwitchPage2(){ switchToPage(1); }
void QtDAQ::onSwitchPage3(){ switchToPage(2); }
void QtDAQ::onSwitchPage4(){ switchToPage(3); }

void QtDAQ::moveCurrentWindowToPage(int page)
{
	if (page < 0 || page >= NUM_UI_PAGES)
		return;

	int activeTabIndex = ui.tabWidget->currentIndex();
	if (activeTabIndex == page || activeTabIndex < 0 || activeTabIndex >= NUM_UI_PAGES)
		return;

	QMdiSubWindow* currentSubWindow = mdiAreas[activeTabIndex]->activeSubWindow();
	if (!currentSubWindow)
		return;

	QWidget* windowWidget = currentSubWindow->widget();
	if (!windowWidget)
		return;
	PlotWindow* plotWindow = dynamic_cast<PlotWindow*>(windowWidget);
	if (!plotWindow)
		return;
	plotWindow->tabIndex = page;
	mdiAreas[activeTabIndex]->removeSubWindow(currentSubWindow);
	QMdiSubWindow* newSubWindow = mdiAreas[page]->addSubWindow(windowWidget);
	windowWidget->setParent(newSubWindow);
	SAFE_DELETE(currentSubWindow);

}

void QtDAQ::switchToPage(int page)
{
	if (page < 0 || page >= NUM_UI_PAGES)
		return;

	ui.tabWidget->setCurrentIndex(page);
}

void QtDAQ::onRenamePageClicked()
{
	int activeTabIndex = ui.tabWidget->currentIndex();
	if (activeTabIndex < 0 || activeTabIndex >= NUM_UI_PAGES)
		return;
	bool ok;
	QString text = QInputDialog::getText(this, "Page heading",
		"New heading:", QLineEdit::Normal,
		ui.tabWidget->tabText(activeTabIndex), &ok);
	if (ok && !text.isEmpty())
		ui.tabWidget->setTabText(activeTabIndex, text);
}

void QtDAQ::closeEvent(QCloseEvent*)
{
	if (vxAcquisitionThread)
	{
		vxAcquisitionThread->stopAcquisition(true);
		vxAcquisitionThread->disconnect();
		vxAcquisitionThread->exit();
		vxAcquisitionThread->wait(100);
		vxAcquisitionThread->terminate();
		SAFE_DELETE(vxAcquisitionThread);
	}


	if (rawBuffer1Mutex && !finishedReading)
	{
		rawBuffer1Mutex->lock();
		rawBuffer2Mutex->lock();
	}
	
	if (drsReaderThread)
	{
		if (!finishedReading)
			drsReaderThread->stopReading(true);
		drsReaderThread->exit();
		drsReaderThread->wait(100);
		drsReaderThread->terminate();
		SAFE_DELETE(drsReaderThread);
	}

	if (drsAcquisitionThread)
	{
		drsAcquisitionThread->stopAcquisition();
		drsAcquisitionThread->exit();
		drsAcquisitionThread->wait(100);
		drsAcquisitionThread->terminate();
		SAFE_DELETE(drsAcquisitionThread);
	}

	if (vxReaderThread)
	{
		vxReaderThread->stopReading(true);
		vxReaderThread->disconnect();
		vxReaderThread->exit();
		vxReaderThread->wait(100);
		vxReaderThread->terminate();
		SAFE_DELETE(vxReaderThread);
	}


	if (vxProcessThread)
	{
		vxProcessThread->disconnect();
		vxProcessThread->exit();
		vxProcessThread->wait(100);
		vxProcessThread->terminate();
		SAFE_DELETE(vxProcessThread);
	}

	
	if (rawBuffer1)
	{
		for (size_t i = 0; i < bufferLength; i++)
			freeVxEvent(rawBuffer1[i]);
	}
	if (rawBuffer2)
	{
		for (size_t i = 0; i < bufferLength; i++)
			freeVxEvent(rawBuffer2[i]);
	}
	
	SAFE_DELETE_ARRAY(rawBuffer1);
	//if (rawBuffer1Mutex)
	//	rawBuffer1Mutex->unlock();
	SAFE_DELETE_ARRAY(rawBuffer2);
	//if (rawBuffer2Mutex)
	//	rawBuffer2Mutex->unlock();
	//SAFE_DELETE(board);
	SAFE_DELETE(drs);
	SAFE_DELETE(acquisitionConfig);
	SAFE_DELETE(analysisConfig);
	SAFE_DELETE(vxConfig);
}

void QtDAQ::onProjectorModeToggled(bool checked)
{
	projectorMode = checked;
	statusBar()->setHidden(checked);
	for (auto& i : signalPlots)
		i->setProjectorMode(projectorMode);

	for (auto& i : histograms)
		i->setProjectorMode(projectorMode);

	for (auto& i : histograms2D)
		i->setProjectorMode(projectorMode);

	for (auto& i : fomPlots)
		i->setProjectorMode(projectorMode);

	for (auto& i : sortedPairPlots)
		i->setProjectorMode(projectorMode);

}

void QtDAQ::onFilePercentUpdated(float filePercent)
{
#ifdef WIN32
	progressTaskbar->setValue((int)(round(filePercent * 100))%100);
#endif
	this->filePercent = filePercent;
}

void QtDAQ::onLoadTemperatureLogClicked()
{
	QFileDialog fileDialog(this, "Set input file", "", "Log files (*.txt);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (!rawFilename.isEmpty())
	{
		QFileInfo fileInfo(rawFilename);
		fileDialog.setDirectory(fileInfo.dir().absolutePath());
		fileDialog.selectFile(fileInfo.fileName() + ".txt");
	}
	else
	{
		QString prevFileDir = settings.value("analysis/prevLogDir").toString();
		QString prevFile = settings.value("analysis/prevLogFile").toString();
		fileDialog.setDirectory(prevFileDir);
	}

	if (fileDialog.exec())
	{
		temperatureLogFilename = fileDialog.selectedFiles().first();
		QFileInfo fileInfo(temperatureLogFilename);
		settings.setValue("analysis/prevLogDir", fileInfo.dir().absolutePath());
		settings.setValue("analysis/prevLogFile", fileInfo.fileName());
		if (vxProcessThread)
			vxProcessThread->loadTemperatureLog(temperatureLogFilename);
	}

}
