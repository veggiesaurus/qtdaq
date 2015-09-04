#pragma once

#include <QDialog>
#include <QVector>
#include <QComboBox>
#include "ui_dialogActiveCuts.h"
#include "Cuts.h"

class ActiveCutsDialog : public QDialog
{
	Q_OBJECT
public:
	ActiveCutsDialog (QVector<LinearCut>& cuts, QVector<PolygonalCut>& polygonalCuts, QVector<Condition>conditions, QVector<Condition>polygonalConditions, QWidget * parent = 0, Qt::WindowFlags f = 0 );
private:

public:
	Ui::DialogActiveCuts ui;
	QVector<QComboBox*> comboBoxesConditions;
public slots:

};