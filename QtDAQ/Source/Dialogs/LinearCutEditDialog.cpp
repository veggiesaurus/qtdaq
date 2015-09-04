#include "globals.h"
#include "Dialogs/LinearCutEditDialog.h"

LinearCutEditDialog::LinearCutEditDialog (QVector<LinearCut>& s_cuts, bool s_isRemoveDialog, QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f), cuts(s_cuts), isRemoveDialog(s_isRemoveDialog)
{
	ui.setupUi(this);
	QVectorIterator<LinearCut> i(cuts);
	while (i.hasNext())
	{
		ui.comboBoxName->addItem(i.next().name);
	}
	if (cuts.size())
		updateFields(cuts[0]);
	else
		ui.comboBoxName->addItem("No LinearCuts");
	if (isRemoveDialog)
	{
		setWindowTitle("Remove Linear Cut");
		ui.okButton->setText("Remove");
		ui.doubleSpinBoxMax->setEnabled(false);
		ui.doubleSpinBoxMin->setEnabled(false);
	}
}

void LinearCutEditDialog::updateFields(LinearCut& cut)
{
	ui.comboBoxParameter->setCurrentIndex((int)cut.parameter);
	ui.comboBoxChannel->setCurrentIndex((int)cut.channel+1);
	ui.doubleSpinBoxMin->setValue(cut.cutMin);
	ui.doubleSpinBoxMax->setValue(cut.cutMax);
	onLinearCutChanged();
}


void LinearCutEditDialog::onIndexChanged(int index)
{
	if (cuts.size())
	{
		updateFields(cuts[index]);
	}
}

void LinearCutEditDialog::onLinearCutChanged()
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