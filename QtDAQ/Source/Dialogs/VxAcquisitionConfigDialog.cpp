#include <QMessageBox>
#include <QDebug>
#include "globals.h"
#include "VxAcquisitionConfig.h"
#include "VxAcquisitionConfigDialog.h"

VxAcquisitionConfigDialog::VxAcquisitionConfigDialog(QString s_config, QWidget * parent, Qt::WindowFlags f) : config(s_config)
{
	ui.setupUi(this);
	ui.plainTextEditCode->setPlainText(config);
	highlighter = new VxHighlighter(ui.plainTextEditCode->document());
}

void VxAcquisitionConfigDialog::configChanged()
{
	QPlainTextEdit* configEdit = dynamic_cast<QPlainTextEdit*>(sender());
	disconnect(configEdit, SIGNAL(textChanged()), 0, 0);

	QTextCharFormat highlightFormatCritical;
	highlightFormatCritical.setUnderlineColor(Qt::darkRed);
	highlightFormatCritical.setUnderlineStyle(QTextCharFormat::WaveUnderline);

	QTextCharFormat highlightFormatNonCritical;
	highlightFormatNonCritical.setUnderlineColor(QColor(255,174,185));
	highlightFormatNonCritical.setUnderlineStyle(QTextCharFormat::WaveUnderline);

	QTextCharFormat highlightFormatWarning;
	highlightFormatWarning.setUnderlineColor(QColor(255,165,0));
	highlightFormatWarning.setUnderlineStyle(QTextCharFormat::WaveUnderline);

	QTextCursor cursor(configEdit->document());
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	QTextCharFormat highlightFormat;
	highlightFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
	cursor.mergeCharFormat(highlightFormat);

	QVector<VxParseError> parseErrors;
	VxAcquisitionConfig* updatedConfig = VxAcquisitionConfig::parseConfigString(ui.plainTextEditCode->toPlainText(), parseErrors);
	for each (auto parseError in parseErrors)
	{
		switch (parseError.errorType)
		{
		case VxParseError::CRITICAL:
			underlineError(configEdit, parseError.lineNumber-1, highlightFormatCritical);
			break;
		case VxParseError::NON_CRITICAL:
			underlineError(configEdit, parseError.lineNumber-1, highlightFormatNonCritical);
			break;
		case VxParseError::WARNING:
			underlineError(configEdit, parseError.lineNumber-1, highlightFormatWarning);
			break;
		default:
			break;
		}
	}
	SAFE_DELETE(updatedConfig);

	connect(configEdit, SIGNAL(textChanged()), this, SLOT(configChanged()), Qt::QueuedConnection);

}


void VxAcquisitionConfigDialog::underlineError(QPlainTextEdit* textEdit, int lineNum, QTextCharFormat highlightFormat)
{
	QTextBlock block = textEdit->document()->findBlockByLineNumber(lineNum);
	QTextCursor errorLine(block);
	errorLine.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);	
	errorLine.mergeCharFormat(highlightFormat);
}