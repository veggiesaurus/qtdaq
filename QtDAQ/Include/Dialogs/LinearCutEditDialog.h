#pragma once

#include <QDialog>
#include <QVector>
#include "ui_dialogLinearCutEdit.h"
#include "Cuts.h"

class LinearCutEditDialog : public QDialog
{
        Q_OBJECT
public:
        LinearCutEditDialog (QVector<LinearCut>& s_cuts, bool s_isRemoveDialog=false, QWidget * parent = 0, Qt::WindowFlags f = 0);
private:
        void updateFields(LinearCut& cut);
public:
        Ui::DialogLinearCutEdit ui;
private:
        QVector<LinearCut>& cuts;
		bool isRemoveDialog;
public slots:
        void onIndexChanged(int index);
        void onLinearCutChanged();
};