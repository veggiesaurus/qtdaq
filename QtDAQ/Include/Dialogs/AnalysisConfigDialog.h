#pragma once

#include <QDialog>
#include "ui_dialogAnalysisConfig.h"
#include "AnalysisConfig.h"
#include "JSHighlighter.h"
#include "../V8/V8Wrapper.h"

class AnalysisConfigDialog : public QDialog
{
	Q_OBJECT
public:
	AnalysisConfigDialog (AnalysisConfig* s_config, QWidget * parent = 0, Qt::WindowFlags f = 0 );
	void updateConfig();
private:
	void setUIFromConfig(AnalysisConfig* s_config);
	void underlineError(QPlainTextEdit* textEdit, int lineNum);

private slots:
	void updateUI();
	void codeChanged();
signals:
	void analysisConfigChanged(AnalysisConfig*);
private:
	Ui::DialogAnalysisConfig ui;
	JSHighlighter* highlighters[6];
	AnalysisConfig* config;

	//v8
	Isolate* isolate;

};
