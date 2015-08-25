#include "VxAcquisitionConfigDialog.h"

VxAcquisitionConfigDialog::VxAcquisitionConfigDialog(QString s_config, QWidget * parent, Qt::WindowFlags f) : config(s_config)
{
	ui.setupUi(this);
	ui.plainTextEditCode->setPlainText(config);
	highlighter = new VxHighlighter(ui.plainTextEditCode->document());
}

void VxAcquisitionConfigDialog::configChanged()
{
	qDebug() << "Config changed";
}
