#include "qtdaq.h"


QtDAQ::QtDAQ(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	
	mdiAreas[0] = ui.mdiArea1;
	mdiAreas[1] = ui.mdiArea2;
	mdiAreas[2] = ui.mdiArea3;
	mdiAreas[3] = ui.mdiArea4;

	showMaximized();
	config = AcquisitionConfig::DefaultConfig();
	analysisConfig = new AnalysisConfig();
	qRegisterMetaType<AnalysisConfig>("AnalysisConfig");
	qRegisterMetaTypeStreamOperators<AnalysisConfig>("AnalysisConfig");

	//restore previous settings
	QVariant var = settings.value("analysis/previousSettings");
	if (var.isValid())
		*analysisConfig = var.value<AnalysisConfig>();


	//buffers and mutexes
	//raw
	rawBuffer1Mutex = 0;
	rawBuffer2Mutex = 0;
	rawBuffer1 = new EventVx[EVENT_BUFFER_SIZE];
	rawBuffer2 = new EventVx[EVENT_BUFFER_SIZE];
	memset(rawBuffer1, 0, sizeof(EventVx)*EVENT_BUFFER_SIZE);
	memset(rawBuffer2, 0, sizeof(EventVx)*EVENT_BUFFER_SIZE);
	//processed
	//raw
	procBuffer1Mutex = new QMutex();
	procBuffer2Mutex = new QMutex();
	procBuffer1 = new EventStatistics[EVENT_BUFFER_SIZE];
	procBuffer2 = new EventStatistics[EVENT_BUFFER_SIZE];
	memset(procBuffer1, 0, sizeof(EventStatistics)*EVENT_BUFFER_SIZE);
	memset(procBuffer2, 0, sizeof(EventStatistics)*EVENT_BUFFER_SIZE);

	vxReaderThread = NULL;
	vxProcessThread = NULL;

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
	drs = NULL;
	board = NULL;

	serial = NULL;
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

	ui.menuView->removeAction(ui.menuSwitchPage->menuAction());
	//ui.menuSwitchPage->setVisible(false);
	new QShortcut(QKeySequence(Qt::Key_1), this, SLOT(onSwitchPage1()));
	new QShortcut(QKeySequence(Qt::Key_2), this, SLOT(onSwitchPage2()));
	new QShortcut(QKeySequence(Qt::Key_3), this, SLOT(onSwitchPage3()));
	new QShortcut(QKeySequence(Qt::Key_4), this, SLOT(onSwitchPage4()));

	acquisitionTime = new QTime();
	//update timer for ui
	uiUpdateTimer = new QTimer();
	uiUpdateTimer->setInterval(1000);
	numEventsProcessed = 0;
	prevNumEventsProcessed = 0;
	numUITimerTimeouts = 0;
	connect(uiUpdateTimer, SIGNAL(timeout()), this, SLOT(onUiUpdateTimerTimeout()));
}

QtDAQ::~QtDAQ()
{
	delete config;
}

void QtDAQ::onUiUpdateTimerTimeout()
{
	if (!rawFilename.isEmpty())
	{
		setWindowTitle("SolarDAQ - " + rawFilename);
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
			int timeMillis = uiUpdateTimer->interval();
			double rate = ((double)max(0, numEventsProcessed - prevNumEventsProcessed)) / timeMillis*1000.0;
			prevNumEventsProcessed = numEventsProcessed;
			numUITimerTimeouts++;
			statusBar()->showMessage("Processing data: " + QString::number(numEventsProcessed) + " events processed @ " + QString::number(rate) + " events/s", uiUpdateTimer->interval());
		}
	}

}

void QtDAQ::onEventReadingFinished()
{
	prevAcquisitionTime = acquisitionTime->elapsed();

	finishedReading = true;

}

void QtDAQ::onDRSObjectChanged(DRS* s_drs, DRSBoard* s_board)
{
	if (s_board != NULL)
	{
		drs = s_drs;
		board = s_board;
		ui.actionInit->setEnabled(true);
	}
	else
	{
		ui.actionInit->setEnabled(false);
		ui.actionStart->setEnabled(false);
		ui.actionStop->setEnabled(false);
		ui.actionReset->setEnabled(false);
	}
}



void QtDAQ::onNewProcessedEvents(QVector<EventStatistics*>* stats)
{
	//return;
	if (!stats)
		return;
	numEventsProcessed += stats->size();

	for (QVector<HistogramWindow*>::iterator it = histograms.begin(); it != histograms.end(); it++)
	{
		(*it)->onNewEventStatistics(stats);
	}

	for (QVector<Histogram2DWindow*>::iterator it = histograms2D.begin(); it != histograms2D.end(); it++)
	{
		(*it)->onNewEventStatistics(stats);
	}

	for (QVector<FoMWindow*>::iterator it = fomPlots.begin(); it != fomPlots.end(); it++)
	{
		(*it)->onNewEventStatistics(stats);
	}

	for (QVector<SortedPairPlotWindow*>::iterator it = sortedPairPlots.begin(); it != sortedPairPlots.end(); it++)
	{
		(*it)->onNewEventStatistics(stats);
	}

	for (QVector<EventStatistics*>::iterator it = stats->begin(); it != stats->end(); it++)
	{
		EventStatistics* statEvent = *it;
		SAFE_DELETE(statEvent);
	}
	stats->clear();
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
}

void QtDAQ::onReadDRSFileClicked()
{
	QFileDialog fileDialog(this, "Set raw data input file", "", "Uncompressed data (*.dat);;Compressed data (*.dtz);;All files (*.*)");
	fileDialog.restoreState(settings.value("mainWindow/loadRawState").toByteArray());
	fileDialog.selectFile(settings.value("mainWindow/rawDataDirectory").toString());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (fileDialog.exec())
	{
		QString newRawFilename = fileDialog.selectedFiles().first();
		settings.setValue("mainWindow/loadRawState", fileDialog.saveState());
		QDir currentDir;
		settings.setValue("mainWindow/rawDataDirectory", currentDir.absoluteFilePath(newRawFilename));

		rawFilename = newRawFilename;
		clearAllPlots();
		bool compressedOutput = rawFilename.endsWith("dtz");

		drsReaderThread->initDRSBinaryReaderThread(rawFilename, compressedOutput, analysisConfig);
		drsReaderThread->start();
		uiUpdateTimer->stop();
		numUITimerTimeouts = 0;
		uiUpdateTimer->start();
		acquisitionTime->start();
		dataMode = DRS_MODE;
	}

}

void QtDAQ::onReadVx1761FileClicked()
{
	QFileDialog fileDialog(this, "Set raw data input file", "", "Compressed data (*.dtz);;All files (*.*)");
	fileDialog.restoreState(settings.value("mainWindow/loadRawState").toByteArray());
	fileDialog.selectFile(settings.value("mainWindow/rawDataDirectory").toString());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (fileDialog.exec())
	{
		QString newRawFilename = fileDialog.selectedFiles().first();
		settings.setValue("mainWindow/loadRawState", fileDialog.saveState());
		QDir currentDir;
		settings.setValue("mainWindow/rawDataDirectory", currentDir.absoluteFilePath(newRawFilename));
		rawFilename = newRawFilename;
		clearAllPlots();

		bool compressedOutput = rawFilename.endsWith("dtz");

		SAFE_DELETE(rawBuffer1Mutex);
		SAFE_DELETE(rawBuffer2Mutex);
		rawBuffer1Mutex = new QMutex();
		rawBuffer2Mutex = new QMutex();

		if (vxProcessThread)
		{
			vxProcessThread->disconnect();
			delete vxProcessThread;
		}
		vxProcessThread = new VxProcessThread(rawBuffer1Mutex, rawBuffer2Mutex, rawBuffer1, rawBuffer2, procBuffer1Mutex, procBuffer2Mutex, procBuffer1, procBuffer2, this);
		connect(this, SIGNAL(resumeProcessing()), vxProcessThread, SLOT(onResumeProcessing()), Qt::QueuedConnection);
		connect(vxProcessThread, SIGNAL(newProcessedEvents(QVector<EventStatistics*>*)), this, SLOT(onNewProcessedEvents(QVector<EventStatistics*>*)), Qt::QueuedConnection);
		connect(vxProcessThread, SIGNAL(newEventSample(EventSampleData*)), this, SLOT(onNewEventSample(EventSampleData*)), Qt::QueuedConnection);
		vxProcessThread->initVxProcessThread(analysisConfig);


		if (vxReaderThread)
		{
			vxReaderThread->stopReading(true);
			//vxReaderThread->exit();
			vxReaderThread->disconnect();
			delete vxReaderThread;
		}
		vxReaderThread = new VxBinaryReaderThread(rawBuffer1Mutex, rawBuffer2Mutex, rawBuffer1, rawBuffer2, this);
		connect(vxReaderThread, SIGNAL(eventReadingFinished()), this, SLOT(onEventReadingFinished()));

		vxReaderThread->initVxBinaryReaderThread(rawFilename, compressedOutput, analysisConfig);
		vxReaderThread->start();
		uiUpdateTimer->stop();
		numUITimerTimeouts = 0;
		numEventsProcessed = 0;
		prevNumEventsProcessed = 0;
		finishedReading = false;
		uiUpdateTimer->start();
		acquisitionTime->start();
		vxProcessThread->start();
		dataMode = Vx1761_MODE;
		ui.actionPause_File_Reading->setChecked(false);
	}

}

void QtDAQ::onReplayCurrentFileClicked()
{
	clearAllPlots();
	if (!rawFilename.isEmpty())
	{
		bool compressedOutput = rawFilename.endsWith("dtz");
		numEventsProcessed = 0;
		prevNumEventsProcessed = 0;
		finishedReading = false;
		ui.actionPause_File_Reading->setChecked(false);
		if (dataMode == DRS_MODE)
		{
			drsReaderThread->initDRSBinaryReaderThread(rawFilename, compressedOutput, analysisConfig);
			drsReaderThread->start();
		}
		else if (dataMode == Vx1761_MODE)
		{
			vxProcessThread->resetTriggerTimerAdjustments();
			vxReaderThread->rewindFile();
			vxReaderThread->setPaused(false);
		}

		uiUpdateTimer->stop();
		numUITimerTimeouts = 0;
		uiUpdateTimer->start();
		acquisitionTime->start();
	}
}

void QtDAQ::onReadStatisticsFileClicked()
{

}


void QtDAQ::onInitClicked()
{
	drsAcquisitionThread->initDRSAcquisitionThread(drs, board, config, analysisConfig);
	ui.actionStart->setEnabled(true);
}



void QtDAQ::onStartClicked()
{
	uiUpdateTimer->start();
	acquisitionTime->start();
	numEventsProcessed = 0;
	drsAcquisitionThread->start(QThread::HighPriority);
	ui.actionStop->setEnabled(true);
	ui.actionReset->setEnabled(true);
}


void QtDAQ::onStopClicked()
{
	drsAcquisitionThread->stopAcquisition();
	drsAcquisitionThread->terminate();
	ui.actionStart->setEnabled(true);
	ui.actionStop->setEnabled(false);
	ui.actionReset->setEnabled(false);
}

void QtDAQ::onResetClicked()
{
	clearAllPlots();
	uiUpdateTimer->start();
	acquisitionTime->start();
	numEventsProcessed = 0;
}

void QtDAQ::onEditConfigClicked()
{
	ConfigDialog* dialog = new ConfigDialog(config);
	connect(dialog, SIGNAL(drsObjectChanged(DRS*, DRSBoard*)), this, SLOT(onDRSObjectChanged(DRS*, DRSBoard*)));
	dialog->setDRS(drs);
	int retDialog = dialog->exec();

	if (retDialog == QDialog::Rejected)
		return;
	dialog->updateConfig();
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
	settings.setValue("analysis/previousSettings", qVariantFromValue(*analysisConfig));
}

void QtDAQ::onLoadConfigClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set config file", "", "Binary config (*.bcfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.restoreState(settings.value("mainWindow/saveConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (!rawFilename.isEmpty())
	{
		fileDialog.selectFile(rawFilename + ".bcfg");
	}
	if (fileDialog.exec())
	{
		settings.setValue("mainWindow/saveConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();	QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
			return;
		file.read((char*)config, sizeof(AcquisitionConfig));
		file.close();
	}
}

void QtDAQ::onLoadAnalysisConfigClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set config file", "", "Analysis config (*.acfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.restoreState(settings.value("mainWindow/saveAnalysisConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (!rawFilename.isEmpty())
	{
		fileDialog.selectFile(rawFilename + ".acfg");
	}
	if (fileDialog.exec())
	{
		settings.setValue("mainWindow/saveAnalysisConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFile file(fileName);

		if (!file.open(QIODevice::ReadOnly))
			return;
		QDataStream stream(&file);
		stream >> (*analysisConfig);
		file.close();
		if (vxProcessThread)
			vxProcessThread->onAnalysisConfigChanged();
		settings.setValue("analysis/previousSettings", qVariantFromValue(*analysisConfig));
	}
}

void QtDAQ::onSaveConfigClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set output file", "", "Binary config (*.bcfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.restoreState(settings.value("mainWindow/saveConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);
	if (!rawFilename.isEmpty())
	{
		fileDialog.selectFile(rawFilename + ".bcfg");
	}
	if (fileDialog.exec())
	{
		settings.setValue("mainWindow/saveConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly))
			return;
		file.write((const char*)config, sizeof(AcquisitionConfig));
		file.close();
	}
}

void QtDAQ::onSaveAnalysisConfigClicked()
{
	//open a file for output
	QFileDialog fileDialog(this, "Set output file", "", "Analysis config (*.acfg);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.restoreState(settings.value("mainWindow/saveAnalysisConfigState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);
	if (!rawFilename.isEmpty())
	{
		fileDialog.selectFile(rawFilename + ".acfg");
	}
	if (fileDialog.exec())
	{
		settings.setValue("mainWindow/saveAnalysisConfigState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
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
	fileDialog.restoreState(settings.value("mainWindow/saveUIState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);
	if (!rawFilename.isEmpty())
	{
		fileDialog.selectFile(rawFilename + ".uic");
	}
	if (fileDialog.exec())
	{
		settings.setValue("mainWindow/saveUIState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly))
			return;

		QDataStream stream(&file);
		stream << saveGeometry();
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

void QtDAQ::onRestoreUIClicked()
{
	//open a file for config input
	QFileDialog fileDialog(this, "Set input file", "", "UI config (*.uic);;All files (*.*)");
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.restoreState(settings.value("mainWindow/saveUIState").toByteArray());
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	if (!rawFilename.isEmpty())
	{
		fileDialog.selectFile(rawFilename + ".uic");
	}
	if (fileDialog.exec())
	{
		qDebug("Test");
		settings.setValue("mainWindow/saveUIState", fileDialog.saveState());
		QString fileName = fileDialog.selectedFiles().first();
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
		for (int i = 0; i<histograms.size(); i++)
			histograms[i]->parentWidget()->close();
		histograms.clear();

		for (int i = 0; i<histograms2D.size(); i++)
			histograms2D[i]->parentWidget()->close();
		histograms2D.clear();

		for (int i = 0; i<fomPlots.size(); i++)
			fomPlots[i]->parentWidget()->close();
		fomPlots.clear();

		for (int i = 0; i<signalPlots.size(); i++)
			signalPlots[i]->parentWidget()->close();
		signalPlots.clear();

		for (int i = 0; i<sortedPairPlots.size(); i++)
			sortedPairPlots[i]->parentWidget()->close();
		sortedPairPlots.clear();

		QDataStream stream(&file);

		QByteArray geometryMainWindow;
		stream >> geometryMainWindow;
		restoreGeometry(geometryMainWindow);
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
			setupPlotWindow(histogram, false);
			stream >> *histogram;
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
			setupPlotWindow(histogram2D, false);
			stream >> *histogram2D;
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
			setupPlotWindow(fomPlot, false);
			stream >> *fomPlot;
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
			setupPlotWindow(signalPlot, false);
			stream >> *signalPlot;
			signalPlots.push_back(signalPlot);

			connect(signalPlot, SIGNAL(plotClosed()), this, SLOT(onSignalPlotClosed()));
		}
		file.close();

		//disable multiple restores
		ui.actionRestore_UI->setEnabled(false);
	}
}

void QtDAQ::onPauseReadingToggled(bool checked)
{
	if (vxReaderThread)
		vxReaderThread->setPaused(checked);
}

void QtDAQ::onCascadeClicked()
{
	ui.mdiArea1->cascadeSubWindows();
}

void QtDAQ::onTileClicked()
{
	ui.mdiArea1->tileSubWindows();
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
	plotWindow->setCutVectors(&cuts, &polygonalCuts, appendConditions);
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
	setupPlotWindow(histogram);
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
	setupPlotWindow(histogram, false);
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
	setupPlotWindow(histogram2D, false);
	histogram2D->onNewCalibration(histogram2D->chPrimary, calibrationValues[histogram2D->chPrimary]);
	histograms2D.push_back(histogram2D);
	connect(histogram2D, SIGNAL(plotClosed()), this, SLOT(onHistogram2DClosed()));
	connect(histogram2D, SIGNAL(channelChanged(int)), this, SLOT(onHistogram2DChannelChanged(int)));
	connect(this, SIGNAL(calibrationChanged(int, EnergyCalibration)), histogram2D, SLOT(onNewCalibration(int, EnergyCalibration)));
	connect(histogram2D, SIGNAL(duplicate(Histogram2DWindow*)), this, SLOT(onHistogram2DDuplication(Histogram2DWindow*)));
}

void QtDAQ::onFoMPlotDuplication()
{

}

void QtDAQ::onSignalPlotDuplication()
{

}

void QtDAQ::onExitClicked()
{
	close();
}

void QtDAQ::onSerialInterfaceClicked()
{
	bool ok;
	QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
	if (list.isEmpty())
	{
		QMessageBox::critical(this, "Port enumeration failed", "Unable to find any available serial ports.");
		return;
	}
	QList<QString> strList;
	QList<QSerialPortInfo>::iterator i;
	for (i = list.begin(); i != list.end(); ++i)
		strList.append(i->portName());
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
	for (QVector<HistogramWindow*>::iterator it = histograms.begin(); it != histograms.end(); it++)
	{
		(*it)->clearValues();
	}

	for (QVector<Histogram2DWindow*>::iterator it = histograms2D.begin(); it != histograms2D.end(); it++)
	{
		(*it)->clearValues();
	}

	for (QVector<FoMWindow*>::iterator it = fomPlots.begin(); it != fomPlots.end(); it++)
	{
		(*it)->clearValues();
	}

	for (QVector<SortedPairPlotWindow*>::iterator it = sortedPairPlots.begin(); it != sortedPairPlots.end(); it++)
	{
		(*it)->clearValues();
	}
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