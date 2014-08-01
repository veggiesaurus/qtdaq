#include "Dialogs/CalibrationDialog.h"



CailbrationDialog::CailbrationDialog (QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	validData=0;
}

void CailbrationDialog::onDataEntryChanged(int xIndex,int yIndex)
{
	QTableWidgetItem* item=ui.tableWidgetData->item(xIndex, yIndex);
	QString entry=item->text();
	bool check=true;
	double val=entry.toDouble(&check);
	if (!check || val<0)	
		item->setBackgroundColor(QColor("pink"));		
	else
	{
		item->setBackgroundColor(QColor("white"));
		//recaluclate values
		calculateCalibrationValues();
	}
}

void CailbrationDialog::calculateCalibrationValues()
{
	int numValidPoints=0;
	int numPoints=ui.tableWidgetData->rowCount();
	double s_x=0;
	double s_y=0;
	double s_x2=0;
	double s_xy=0;

	for (int i=0;i<numPoints;i++)
	{
		if (ui.tableWidgetData->item(i, 0) && ui.tableWidgetData->item(i, 1))
		{
			bool check;
			double x=ui.tableWidgetData->item(i, 0)->text().toDouble(&check);
			if (!check)
				continue;
			double y=ui.tableWidgetData->item(i, 1)->text().toDouble(&check);
			if (!check)
				continue;
			numValidPoints++;
			s_x+=x;
			s_y+=y;
			s_x2+=x*x;
			s_xy+=x*y;
		}
	}

	if (numValidPoints>=2)
	{
		double av_x=s_x/numValidPoints;
		double av_x2=s_x2/numValidPoints;
		double av_xy=s_xy/numValidPoints;
		double av_y=s_y/numValidPoints;

		scale=(av_xy-av_x*av_y)/(av_x2-av_x*av_x);

		ui.labelValidCalib->setText("Calibration valid");
		offset=av_y-scale*av_x;
		ui.lineEditOffset->setText(QString::number(offset));
		ui.lineEditScale->setText(QString::number(scale));
	}
}