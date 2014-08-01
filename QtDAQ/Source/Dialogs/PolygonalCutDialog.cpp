#include "Dialogs/PolygonalCutDialog.h"




PolygonalCutDialog::PolygonalCutDialog (int channel, HistogramParameter parameterX, HistogramParameter parameterY, QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	ui.comboBoxChannel->setCurrentIndex(channel+1);
	ui.comboBoxParameterX->setCurrentIndex(parameterX);
	ui.comboBoxParameterY->setCurrentIndex(parameterY);
	autoName=true;
	autoUpdateName();
}

void PolygonalCutDialog::autoUpdateName()
{
	QString name="cut_";
	//index=0 is all channels
	if (ui.comboBoxChannel->currentIndex()==0)
		name+="active_";
	else
		name+="ch"+QString::number(ui.comboBoxChannel->currentIndex()-1)+"_";

	//parameters
	HistogramParameter parameterX=(HistogramParameter)ui.comboBoxParameterX->currentIndex();
	HistogramParameter parameterY=(HistogramParameter)ui.comboBoxParameterY->currentIndex();
	name+=abbrFromParameter(parameterX)+"_"+abbrFromParameter(parameterY);
	ui.lineEditName->setText(name);
}

void PolygonalCutDialog::onNameChanged()
{
	//no longer auto-update names
	autoName=false;
}

void PolygonalCutDialog::onChannelChanged()
{
	if (autoName)
		autoUpdateName();
}

void PolygonalCutDialog::onRedrawClicked()
{
	close();
	setResult(-1);
}