#include "Windows/PlotWindow.h"

PlotWindow::PlotWindow(QWidget* parent, int s_chPrimary):QMainWindow(parent),
	chPrimary(s_chPrimary)
{
	refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
	refreshTimer->start(1000);
	numEvents=0;
	cuts=NULL;
	polygonalCuts=NULL;

}

void PlotWindow::setCutVectors(QVector<LinearCut>*s_cuts,QVector<PolygonalCut>*s_polygonalCuts, bool updateConditions)
{
	cuts=s_cuts;
	polygonalCuts=s_polygonalCuts;
	if (updateConditions)
	{
		conditions.clear();
		for (int i=0;i<cuts->size();i++)
			conditions.push_back(IGNORE_CUT);
		polygonalConditions.clear();
		for (int i=0;i<polygonalCuts->size();i++)
			polygonalConditions.push_back(IGNORE_CUT);
	}
}

void PlotWindow::onLinearCutAdded(LinearCut& cut, bool updateConditions)
{
	if (updateConditions)
		conditions.push_back(Condition(IGNORE_CUT));	
}

void PlotWindow::onPolygonalCutAdded(PolygonalCut& cut, bool updateConditions)
{
	if (updateConditions)
		polygonalConditions.push_back(Condition(IGNORE_CUT));	
}

void PlotWindow::onCutRemoved(int index, bool isPolygonal)
{
	if (isPolygonal)
		polygonalConditions.remove(index);
	else
		conditions.remove(index);		
}

void PlotWindow::onEditLinearCutClicked()
{
	LinearCutEditDialog cutDlg(*cuts, false, this);
	if (cutDlg.exec()==QDialog::Accepted && cuts->size())
	{
		int cutNum=cutDlg.ui.comboBoxName->currentIndex();
		(*cuts)[cutNum].channel=cutDlg.ui.comboBoxChannel->currentIndex()-1;
		(*cuts)[cutNum].parameter=(HistogramParameter)cutDlg.ui.comboBoxParameter->currentIndex();
		(*cuts)[cutNum].cutMin=cutDlg.ui.doubleSpinBoxMin->value();
		(*cuts)[cutNum].cutMax=cutDlg.ui.doubleSpinBoxMax->value();
	}
}


void PlotWindow::onRemoveLinearCutClicked()
{
	LinearCutEditDialog cutDlg(*cuts, true, this);
	if (cutDlg.exec()==QDialog::Accepted && (*cuts).size())
	{
		int cutNum=cutDlg.ui.comboBoxName->currentIndex();
		emit cutRemoved(cutNum, false);
	}
}

void PlotWindow::onActiveCutsClicked()
{
	//TODO: error check this
	if (!cuts || !polygonalCuts)
		return;
	ActiveCutsDialog* dialog=new ActiveCutsDialog(*cuts, *polygonalCuts, conditions, polygonalConditions, this);
    //uiDialogActiveCuts.setupUi(dialog);

    int retDialog=dialog->exec();
    if (retDialog==QDialog::Rejected)
        return;
	QVectorIterator<QComboBox*> iComboBoxes(dialog->comboBoxesConditions);
	for (int i=0;i<conditions.size();i++)	
		conditions[i]=(Condition)(iComboBoxes.next()->currentIndex()-1);	
	for (int i=0;i<polygonalConditions.size();i++)	
		polygonalConditions[i]=(Condition)(iComboBoxes.next()->currentIndex()-1);
}

void PlotWindow::onSaveImageClicked()
{
	QString filename="export.pdf";
#ifndef QT_NO_FILEDIALOG
    const QList<QByteArray> imageFormats =
        QImageWriter::supportedImageFormats();

    QStringList filter;
    filter += "PDF Documents (*.pdf)";
#ifndef QWT_NO_SVG
    filter += "SVG Documents (*.svg)";
#endif
    filter += "Postscript Documents (*.ps)";

    if ( imageFormats.size() > 0 )
    {
        QString imageFilter("Images (");
        for ( int i = 0; i < imageFormats.size(); i++ )
        {
            if ( i > 0 )
                imageFilter += " ";
            imageFilter += "*.";
            imageFilter += imageFormats[i];
        }
        imageFilter += ")";

        filter += imageFilter;
    }

	//open a file for output
	QFileDialog fileDialog(this, "Set export file", "", filter.join(";;"));
	fileDialog.restoreState(settings.value("plotWindow/saveImageState").toByteArray());
	fileDialog.setFileMode(QFileDialog::AnyFile);

	if (!fileDialog.exec())
		return;
	settings.setValue("plotWindow/saveImageState", fileDialog.saveState());
	filename=fileDialog.selectedFiles().first();

#endif
	if ( !filename.isEmpty() )
    {
        QwtPlotRenderer renderer;		
		//TODO: replace KeepFrames
		//renderer.setLayoutFlag(QwtPlotRenderer::KeepFrames, true);
        renderer.renderDocument(plot, filename, QSizeF(320, 180), 85);
    }
}

void PlotWindow::onRenameClicked()
{
	  bool ok;	  
		QString text = QInputDialog::getText(this, "Set Window Name","Name:", QLineEdit::Normal,"", &ok);
    if (ok && !text.isEmpty())
        name=text;
	setWindowTitle(name);
}

void PlotWindow::closeEvent(QCloseEvent* event)
{
	emit plotClosed();
	QMainWindow::closeEvent(event);
}

bool PlotWindow::isInActiveRegion(EventStatistics* eventStats)
{
	int numCuts=cuts->size();
	for (int i=0;i<numCuts;i++)
	{			
		//skip ignored cuts
		if (conditions[i]==IGNORE_CUT)
			continue;

		//select correct channel
		int ch=(*cuts)[i].channel;
		if (ch==-1)
			ch=chPrimary;
		SampleStatistics stats=eventStats->channelStatistics[ch];

		//cut on include and exclude
		if ((conditions[i]==INCLUDE_REGION && !(*cuts)[i].isInCutRegion(stats)) || (conditions[i]==EXCLUDE_REGION && (*cuts)[i].isInCutRegion(stats)))
		{
			return false;
			break;				
		}
	}

	int numPCuts=polygonalCuts->size();
	for (int i=0;i<numPCuts;i++)
	{
		//skip ignored cuts
		if (polygonalConditions[i]==IGNORE_CUT)
			continue;

		//select correct channel
		int ch=(*polygonalCuts)[i].channel;
		if (ch==-1)
			ch=chPrimary;
		SampleStatistics stats=eventStats->channelStatistics[ch];

		//cut on include and exclude
		if ((polygonalConditions[i]==INCLUDE_REGION && !(*polygonalCuts)[i].isInCutRegion(stats)) || (polygonalConditions[i]==EXCLUDE_REGION && (*polygonalCuts)[i].isInCutRegion(stats)))
		{
			return false;
			break;				
		}
	}
	return true;
}

QDataStream &operator<<(QDataStream &out, const PlotWindow &obj)
{
	out<<UI_SAVE_VERSION;
	out<<obj.chPrimary<<obj.conditions<<obj.polygonalConditions;
	out<<obj.name;
	QByteArray geometry=obj.parentWidget()->saveGeometry();
	out<<geometry;
	return out;	
}

QDataStream &operator>>(QDataStream &in, PlotWindow &obj)
{
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	in>>obj.chPrimary>>obj.conditions>>obj.polygonalConditions;
	if (uiSaveVersion>=0x02)
		in>>obj.name;
	QByteArray geometry;
	in>>geometry;
	obj.parentWidget()->restoreGeometry(geometry);
	return in;
}