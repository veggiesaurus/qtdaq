#include "globals.h"
#include "Dialogs/ActiveCutsDialog.h"

ActiveCutsDialog::ActiveCutsDialog (QVector<LinearCut>& cuts, QVector<PolygonalCut>& polygonalCuts, QVector<Condition>conditions, QVector<Condition>polygonalConditions, QWidget * parent, Qt::WindowFlags)
{
	ui.setupUi(this);
	//index starts at 2 because of label and vertical line
	int index=2;
	QVectorIterator<LinearCut> iLinearCuts(cuts);
	QVectorIterator<Condition> iLinearConditions(conditions);
	if (cuts.count()!=conditions.count())
	{
		//TODO: handle this issue
		return;
	}
	

	//linear cuts
	while (iLinearCuts.hasNext())
	{
		LinearCut cut=iLinearCuts.next();
		Condition condition=(iLinearConditions.next());

		ui.gridLayout->addWidget(new QLabel(cut.name, this), index, 0, 1, 1);
		QString parameterName=axisNameFromParameter(cut.parameter);

		if (cut.channel==-1)
			parameterName+=" (Active)";
		else
			parameterName+=" (Ch"+QString::number(cut.channel)+")";


		ui.gridLayout->addWidget(new QLabel(parameterName, this), index, 1, 1, 1);
		QComboBox* comboBoxIncludeExclude=new QComboBox(this);
		comboBoxIncludeExclude->addItem("Exclude");
		comboBoxIncludeExclude->addItem("Ignore");
		comboBoxIncludeExclude->addItem("Include");
		//-1 is exclude, 0 is ignore, 1 is include. Need to shift from [-1,1]->[0,2]
		comboBoxIncludeExclude->setCurrentIndex(condition+1);
		comboBoxesConditions.push_back(comboBoxIncludeExclude);
		ui.gridLayout->addWidget(comboBoxIncludeExclude, index, 2, 1, 1);
		index++;
	}

	QVectorIterator<PolygonalCut> iPolygonalCuts(polygonalCuts);
	QVectorIterator<Condition> iPolygonalConditions(polygonalConditions);
	if (polygonalCuts.count()!=polygonalConditions.count())
		return;

	//polygonal cuts
	while (iPolygonalCuts.hasNext())
	{
		PolygonalCut cut=iPolygonalCuts.next();
		Condition condition=(iPolygonalConditions.next());

		ui.gridLayout->addWidget(new QLabel(cut.name, this), index, 0, 1, 1);
		QString parameterName=axisNameFromParameter(cut.parameterX);
		parameterName+=", "+axisNameFromParameter(cut.parameterY);

		if (cut.channel==-1)
			parameterName+=" (Active)";
		else
			parameterName+=" (Ch"+QString::number(cut.channel)+")";


		ui.gridLayout->addWidget(new QLabel(parameterName, this), index, 1, 1, 1);
		QComboBox* comboBoxIncludeExclude=new QComboBox(this);
		comboBoxIncludeExclude->addItem("Exclude");
		comboBoxIncludeExclude->addItem("Ignore");
		comboBoxIncludeExclude->addItem("Include");
		//-1 is exclude, 0 is ignore, 1 is include. Need to shift from [-1,1]->[0,2]
		comboBoxIncludeExclude->setCurrentIndex(condition+1);
		comboBoxesConditions.push_back(comboBoxIncludeExclude);
		ui.gridLayout->addWidget(comboBoxIncludeExclude, index, 2, 1, 1);
		index++;
	}

	ui.gridLayout->removeWidget(ui.lineBottom);
	ui.gridLayout->addWidget(ui.lineBottom, index, 0, 1, 3);
	ui.gridLayout->removeItem(ui.hboxLayout);
	//line between last item and ok/cancel
	ui.gridLayout->addLayout(ui.hboxLayout, index+1, 1, 1, 2);
}