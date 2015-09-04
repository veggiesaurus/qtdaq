#pragma once

#include <QDialog>
#include "ui_dialogPolygonalCutEdit.h"
#include "Cuts.h"

class PolygonalCutEditDialog : public QDialog
{
	Q_OBJECT
public:
	PolygonalCutEditDialog (QVector<PolygonalCut>& s_polycuts, bool s_isRemoveDialog, QWidget * parent = 0, Qt::WindowFlags f = 0);
private:
        void updateFields(PolygonalCut& cut);
public:
	Ui::DialogPolygonalCutEdit ui;
	QPolygonF currentPoly;
signals:
		void redrawClicked();
		void selectedCutChanged(QPolygonF poly);
private:
        QVector<PolygonalCut>& cuts;
		bool isRemoveDialog;
public slots:
        void onIndexChanged(int index);
		void onRedrawClicked();
		void onRedrawComplete(QVector< QPointF > redrawnPoly);
		void updateCut();
		int exec();
};