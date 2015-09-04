#pragma once
#include <QFile>
#include "ui_window2DHistogramPlot.h"
#include "ui_dialogHistogram2DConfig.h"
#include "Windows/PlotWindow.h"
#include "Dialogs/LinearCutDialog.h"
#include "Dialogs/PolygonalCutDialog.h"


class Histogram2DWindow : public PlotWindow
{
	Q_OBJECT

public:
	Histogram2DWindow (QWidget * parent=0, HistogramParameter s_parameterX=LONG_INTEGRAL, HistogramParameter s_parameterY=PSD_FACTOR, int s_chPrimary=0, double s_parameterMinX=0, double s_parameterMaxX=100000, double s_parameterMinY=0, double s_parameterMaxY=100, int s_numBinsX=512, int s_numBinsY=512, bool s_logScale=false, bool s_smoothing=false);
	void setProjectorMode(bool s_projectorMode);
	~Histogram2DWindow();


public slots:
	void onOptionsClicked();
	void onNewCalibration(int channel, EnergyCalibration calibration);
	void onNewEventStatistics(QVector<EventStatistics*>* events);
	
	void onAddPolygonalCutClicked();
	void onEditPolygonalCutClicked();
	void onRemovePolygonalCutClicked();
	void onPolygonPointsSelected(const QVector< QPointF >&pa);
	void onPolygonalCutEditRedrawRequest();
	void onPolygonalCutEditSelectionChanged(QPolygonF poly);
	void onPolygonalCutEditAccepted();

	void onLogColoursToggled(bool logColoursEnabled);
	void onBilinearSmoothingToggled(bool smoothingEnabled);
	void onDuplicateClicked();	
	void onAddLinearCutClicked();

	void timerUpdate();

	void onPerRowToggled(bool checked);
	void onPerColumnToggled(bool checked);

	QVector<double> GetNormalisedValues();
	

	void clearValues();
	void onSaveDataClicked();
	void onSaveDataHEPROClicked();

signals:
	void polygonalCutCreated(PolygonalCut);
	void linearCutCreated(LinearCut);
	void channelChanged(int);
	void duplicate(Histogram2DWindow*);

private:
	void updateTitleAndAxis();
	void recreateHistogram();
	//called after serialization
	void updateSettings();

	Ui::Window2DHistogramPlot ui;
	Ui::DialogHistogram2DConfig uiDialogHistogramConfig;
	PolygonalCutEditDialog* dialogPolygonalCutEdit;

	HistogramParameter parameterX, parameterY;
	int numBinsX, numBinsY;
	double parameterMinX, parameterMinY;
	double parameterMaxX, parameterMaxY;

	bool logScale, smoothing, displayRegions;
	QVector<double>values;
	//normalising
	bool normPerRow = false;
	bool normPerColumn = false;

	QFile* file;
	QTextStream* hackOutput;

private:
	friend QDataStream &operator<<(QDataStream &out, const Histogram2DWindow &obj);
	friend QDataStream &operator>>(QDataStream &in, Histogram2DWindow &obj);
};
