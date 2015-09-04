#include "globals.h"
#include "Dialogs/LinearCutDialog.h"

LinearCutDialog::LinearCutDialog (QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	autoName=true;
	autoUpdateName();
}

void LinearCutDialog::autoUpdateName()
{
	QString name="cut_";
	//index=0 is all channels
	if (ui.comboBoxChannel->currentIndex()==0)
		name+="active_";
	else
		name+="ch"+QString::number(ui.comboBoxChannel->currentIndex()-1)+"_";

	//parameters
	HistogramParameter parameter=(HistogramParameter)ui.comboBoxParameter->currentIndex();
	name+=abbrFromParameter(parameter);
	ui.lineEditName->setText(name);
}

void LinearCutDialog::onNameChanged()
{
	//no longer auto-update names
	autoName=false;
}

void LinearCutDialog::onLinearCutChanged()
{
	if (ui.doubleSpinBoxMax->value()<ui.doubleSpinBoxMin->value())
	{		
		ui.doubleSpinBoxMin->setStyleSheet("QSpinBox{background:pink;}");
		ui.doubleSpinBoxMax->setStyleSheet("QSpinBox{background:pink;}");
	}
	else
	{
		ui.doubleSpinBoxMin->setStyleSheet("");
		ui.doubleSpinBoxMax->setStyleSheet("");
	}
}

void LinearCutDialog::onChannelChanged()
{
	if (autoName)
		autoUpdateName();
}

void LinearCutDialog::onParameterChanged()
{
	if (autoName)
		autoUpdateName();
}