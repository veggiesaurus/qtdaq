#include "VxAcquisitionConfigDialog.h"

VxAcquisitionConfigDialog::VxAcquisitionConfigDialog(QString s_config, QWidget * parent, Qt::WindowFlags f) : config(s_config)
{
	ui.setupUi(this);
	ui.plainTextEditCode->setPlainText(config);
	highlighter = new VxHighlighter(ui.plainTextEditCode->document());
}

void VxAcquisitionConfigDialog::configChanged()
{
	QVector<VxParseError> parseErrors;
	VxAcquisitionConfig* updatedConfig = VxAcquisitionConfig::parseConfigString(ui.plainTextEditCode->toPlainText(), parseErrors);
	for each (auto parseError in parseErrors)
	{
		switch (parseError.errorType)
		{
		case VxParseError::CRITICAL:
			qDebug() << "Critical error on line " << parseError.lineNumber << ": " << parseError.errorMessage;
			break;
		case VxParseError::NON_CRITICAL:
			qDebug() << "Non-critical error on line " << parseError.lineNumber << ": " << parseError.errorMessage;
			break;
		case VxParseError::WARNING:
			qDebug() << "Warning on line " << parseError.lineNumber << ": " << parseError.errorMessage;
			break;
		default:
			break;
		}
	}
	SAFE_DELETE(updatedConfig);
}
