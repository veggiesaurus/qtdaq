#include "globals.h"
#include "Dialogs/PolygonalCutEditDialog.h"

PolygonalCutEditDialog::PolygonalCutEditDialog (QVector<PolygonalCut>& s_polyCuts, bool s_isRemoveDialog, QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f), cuts(s_polyCuts), isRemoveDialog(s_isRemoveDialog)
{
	ui.setupUi(this);
	QVectorIterator<PolygonalCut> i(cuts);
	while (i.hasNext())
	{
		ui.comboBoxName->addItem(i.next().name);
	}
	if (cuts.size())
		onIndexChanged(0);
	else
		ui.comboBoxName->addItem("No Polygonal Cuts");
	if (isRemoveDialog)
	{
		setWindowTitle("Remove Polygonal Cut");
		ui.redrawButton->setVisible(false);
		ui.okButton->setText("Remove");
	}
}

void PolygonalCutEditDialog::updateFields(PolygonalCut& cut)
{
	ui.comboBoxParameterX->setCurrentIndex((int)cut.parameterX);
	ui.comboBoxParameterY->setCurrentIndex((int)cut.parameterY);
	ui.comboBoxChannel->setCurrentIndex((int)cut.channel+1);
}


void PolygonalCutEditDialog::onIndexChanged(int index)
{
	if (cuts.size())
	{
		updateFields(cuts[index]);		
		emit(selectedCutChanged(cuts[index].points));
	}
}

void PolygonalCutEditDialog::onRedrawClicked()
{
	emit(redrawClicked());
	setVisible(false);
}

void PolygonalCutEditDialog::onRedrawComplete(QVector< QPointF > redrawnPoly)
{
	setVisible(true);
	currentPoly=QPolygonF(redrawnPoly);
}

void PolygonalCutEditDialog::updateCut()
{
	if (ui.comboBoxName->currentIndex()<cuts.size())
	{
		cuts[ui.comboBoxName->currentIndex()].points=currentPoly;
	}
}

int PolygonalCutEditDialog::exec()
{
	onIndexChanged(0);
	return QDialog::exec();
}