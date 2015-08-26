#pragma once

#include <QDialog>
#include <QMessageBox>
#include <QDebug>
#include "ui_dialogVxAcquisitionConfig.h"
#include "globals.h"
#include "VxHighlighter.h"
#include "VxAcquisitionConfig.h"

class VxAcquisitionConfigDialog : public QDialog
{
	Q_OBJECT
public:
	VxAcquisitionConfigDialog(QString s_config, QWidget * parent = 0, Qt::WindowFlags f = 0);

public slots:
	void configChanged();

private:
	void underlineError(QPlainTextEdit* textEdit, int lineNum, QTextCharFormat highlightFormat);

public:
	Ui::DialogVxAcquisitionConfig ui;

private:
	QString config;
	VxHighlighter* highlighter;

};
