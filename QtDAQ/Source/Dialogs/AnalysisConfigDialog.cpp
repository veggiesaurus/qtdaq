#include <QCheckBox>
#include <include/libplatform/libplatform.h>
#include "Dialogs/AnalysisConfigDialog.h"

AnalysisConfigDialog::AnalysisConfigDialog (AnalysisConfig* s_config, QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);	
	config=s_config;
	setWindowFlags(Qt::Window);
	
	QFont font;
	font.setFamily("Courier");
	font.setFixedPitch(true);

#pragma region codeFormatting
	ui.plainTextEditDef->setFont(font);
	ui.plainTextEditInitialCode->setFont(font);
	ui.plainTextEditPreAnalysis->setFont(font);
	ui.plainTextEditPostChannel->setFont(font);
	ui.plainTextEditPostEvent->setFont(font);
	ui.plainTextEditFinalCode->setFont(font);
	
	highlighters[0] = new JSHighlighter(ui.plainTextEditDef->document());
	highlighters[1] = new JSHighlighter(ui.plainTextEditInitialCode->document());
	highlighters[2] = new JSHighlighter(ui.plainTextEditPreAnalysis->document());
	highlighters[3] = new JSHighlighter(ui.plainTextEditPostChannel->document());
	highlighters[4] = new JSHighlighter(ui.plainTextEditPostEvent->document());
	highlighters[5] = new JSHighlighter(ui.plainTextEditFinalCode->document());
#pragma endregion

    setUIFromConfig(config);
}

AnalysisConfigDialog::~AnalysisConfigDialog()
{
    //V8::Dispose();
    for (auto h:highlighters)
        SAFE_DELETE(h);
}
/////////////////////////////////////////////////
// initialises the UI based on a config object
/////////////////////////////////////////////////
void AnalysisConfigDialog::setUIFromConfig(AnalysisConfig* s_config)
{
#pragma region baseline
	//////////////////////////////
	//baseline
	//////////////////////////////
	ui.doubleSpinBoxGain->setValue(s_config->digitalGain);
	ui.spinBoxSampleSize->setValue(s_config->baselineSampleSize);
	ui.spinBoxSampleRange->setValue(s_config->baselineSampleRange);	
#pragma endregion

#pragma region filtering (low pass settings)
	//////////////////////////////
	//filtering
	//////////////////////////////
	ui.checkBoxPreCFD->setChecked(s_config->preCFDFilter);
	ui.checkBoxPostCFD->setChecked(s_config->postCFDFilter);
	ui.doubleSpinBoxPreCFDFactor->setValue(s_config->preCFDFactor);
	ui.doubleSpinBoxPostCFDFactor->setValue(s_config->postCFDFactor);
#pragma endregion

#pragma region integrals
	//////////////////////////////
	// charge integrals
	//////////////////////////////
	ui.comboBoxRelative->setCurrentIndex((int)s_config->timeOffset);
	ui.doubleSpinBoxStartGate->setValue(s_config->startGate);
	ui.doubleSpinBoxShortEnd->setValue(s_config->shortGate);
	ui.doubleSpinBoxLongEnd->setValue(s_config->longGate);
#pragma endregion
	
#pragma region CFD
	//////////////////////////////
	// CFD
	//////////////////////////////
	ui.doubleSpinBoxF->setValue(s_config->CFDFraction);
	ui.spinBoxL->setValue(s_config->CFDLength);
	ui.spinBoxD->setValue(s_config->CFDOffset);
	ui.checkBoxTimingInfo->setChecked(s_config->useTimeCFD);
#pragma endregion

#pragma region Temperature
	//////////////////////////////
	// Temperature
	//////////////////////////////
	ui.checkBoxOfflineCorrectionTemp->setChecked(s_config->useTempCorrection);
	ui.doubleSpinBoxRefTemp->setValue(s_config->referenceTemperature);
	ui.doubleSpinBoxTempAdjustment->setValue(s_config->scalingVariation);
#pragma endregion

#pragma region Saturation
	//////////////////////////////
	// Saturation
	//////////////////////////////
	ui.checkBoxOfflineCorrectionSat->setChecked(s_config->useSaturationCorrection);
	ui.spinBoxSaturationLimit->setValue(s_config->saturationLimitQL);
	ui.doubleSpinBoxPDE->setValue(s_config->PDE);
	ui.spinBoxNumPixels->setValue(s_config->numPixels);
#pragma endregion

#pragma region TimeOfFlight
	//////////////////////////////
	// Time Of Flight
	//////////////////////////////
	ui.checkBoxTimeOfFlight->setChecked(s_config->useTimeOfFlight);
	ui.spinBoxTOFPulseChannel->setValue(s_config->stopPulseChannel);
	ui.comboBoxRelativePulse->setCurrentIndex((int)s_config->timeOffsetPulse);
	ui.spinBoxStartGatePulse->setValue(s_config->startOffSetPulse);
	ui.spinBoxPulseThreshold->setValue(s_config->stopPulseThreshold);
#pragma endregion

#pragma region CustomCode
	//////////////////////////////
	// Cutom code
	//////////////////////////////
	ui.checkBoxInitialEnabled->setChecked(s_config->bInitialV8);
	ui.checkBoxFinalEnabled->setChecked(s_config->bFinalV8);
	ui.checkBoxPreAnalysisEnabled->setChecked(s_config->bPreAnalysisV8);
	ui.checkBoxPostChannelEnabled->setChecked(s_config->bPostChannelV8);
	ui.checkBoxPostEventEnabled->setChecked(s_config->bPostEventV8);
	ui.checkBoxDefEnabled->setChecked(s_config->bDefV8);

	ui.plainTextEditInitialCode->setPlainText(s_config->customCodeInitial);
	ui.plainTextEditFinalCode->setPlainText(s_config->customCodeFinal);
	ui.plainTextEditPreAnalysis->setPlainText(s_config->customCodePreAnalysis);
	ui.plainTextEditPostChannel->setPlainText(s_config->customCodePostChannel);
	ui.plainTextEditPostEvent->setPlainText(s_config->customCodePostEvent);
	ui.plainTextEditDef->setPlainText(s_config->customCodeDef);

	ui.plainTextEditInitialCode->setEnabled(s_config->bInitialV8);
	ui.plainTextEditFinalCode->setEnabled(s_config->bFinalV8);
	ui.plainTextEditPreAnalysis->setEnabled(s_config->bPreAnalysisV8);
	ui.plainTextEditPostChannel->setEnabled(s_config->bPostChannelV8);
	ui.plainTextEditPostEvent->setEnabled(s_config->bPostEventV8);
	ui.plainTextEditDef->setEnabled(s_config->bDefV8);
#pragma endregion

	updateUI();
}



/////////////////////////////////////////////////
// updates the config based on UI
/////////////////////////////////////////////////
void AnalysisConfigDialog::updateConfig()
{
#pragma region baseline
	//////////////////////////////
	//baseline
	//////////////////////////////
	config->digitalGain=ui.doubleSpinBoxGain->value();
	config->baselineSampleSize=ui.spinBoxSampleSize->value();
	config->baselineSampleRange=ui.spinBoxSampleRange->value();
#pragma endregion

#pragma region filtering (low pass settings)
	//////////////////////////////
	//filtering
	//////////////////////////////
	config->preCFDFilter=ui.checkBoxPreCFD->isChecked();
	config->postCFDFilter=ui.checkBoxPostCFD->isChecked();
	config->preCFDFactor=ui.doubleSpinBoxPreCFDFactor->value();
	config->postCFDFactor=ui.doubleSpinBoxPostCFDFactor->value();
#pragma endregion

#pragma region integrals
	//////////////////////////////
	// charge integrals
	//////////////////////////////
	config->timeOffset=(RelativeTimeOffset)ui.comboBoxRelative->currentIndex();
	config->startGate=ui.doubleSpinBoxStartGate->value();
	config->shortGate=ui.doubleSpinBoxShortEnd->value();
	config->longGate=ui.doubleSpinBoxLongEnd->value();
#pragma endregion
	
#pragma region CFD
	//////////////////////////////
	// CFD
	//////////////////////////////
	config->CFDFraction=ui.doubleSpinBoxF->value();
	config->CFDLength=ui.spinBoxL->value();
	config->CFDOffset=ui.spinBoxD->value();
	config->useTimeCFD=ui.checkBoxTimingInfo->isChecked();
#pragma endregion

#pragma region Temperature
	//////////////////////////////
	// Temperature
	//////////////////////////////
	config->useTempCorrection=ui.checkBoxOfflineCorrectionTemp->isChecked();
	config->referenceTemperature=ui.doubleSpinBoxRefTemp->value();
	config->scalingVariation=ui.doubleSpinBoxTempAdjustment->value();
#pragma endregion

#pragma region Saturation
	//////////////////////////////
	// Saturation
	//////////////////////////////
	config->useSaturationCorrection=ui.checkBoxOfflineCorrectionSat->isChecked();
	config->saturationLimitQL=ui.spinBoxSaturationLimit->value();
	config->PDE=ui.doubleSpinBoxPDE->value();
	config->numPixels=ui.spinBoxNumPixels->value();
#pragma endregion

	
#pragma region TimeOfFlight
	//////////////////////////////
	// Time Of Flight
	//////////////////////////////
	config->useTimeOfFlight=ui.checkBoxTimeOfFlight->isChecked();
	config->stopPulseChannel=ui.spinBoxTOFPulseChannel->value();
	config->timeOffsetPulse=(RelativeTimeOffset)ui.comboBoxRelativePulse->currentIndex();
	config->startOffSetPulse=ui.spinBoxStartGatePulse->value();
	config->stopPulseThreshold=ui.spinBoxPulseThreshold->value();
#pragma endregion

#pragma region CustomCode
	//////////////////////////////
	// Cutom code
	//////////////////////////////
	config->bInitialV8=ui.checkBoxInitialEnabled->isChecked();
	config->bFinalV8=ui.checkBoxFinalEnabled->isChecked();
	config->bPreAnalysisV8=ui.checkBoxPreAnalysisEnabled->isChecked();
	config->bPostChannelV8=ui.checkBoxPostChannelEnabled->isChecked();
	config->bPostEventV8=ui.checkBoxPostEventEnabled->isChecked();
	config->bDefV8=ui.checkBoxDefEnabled->isChecked();

	config->customCodeInitial=ui.plainTextEditInitialCode->toPlainText();
	config->customCodeFinal=ui.plainTextEditFinalCode->toPlainText();
	config->customCodePreAnalysis=ui.plainTextEditPreAnalysis->toPlainText();
	config->customCodePostChannel=ui.plainTextEditPostChannel->toPlainText();
	config->customCodePostEvent=ui.plainTextEditPostEvent->toPlainText();
	config->customCodeDef=ui.plainTextEditDef->toPlainText();
#pragma endregion
}

#pragma region slots (slots for ui interactions)

void AnalysisConfigDialog::updateUI()
{		
	ui.doubleSpinBoxPreCFDFactor->setEnabled(ui.checkBoxPreCFD->isChecked());
	ui.doubleSpinBoxPostCFDFactor->setEnabled(ui.checkBoxPostCFD->isChecked());
	ui.doubleSpinBoxRefTemp->setEnabled(ui.checkBoxOfflineCorrectionTemp->isChecked());
	ui.doubleSpinBoxTempAdjustment->setEnabled(ui.checkBoxOfflineCorrectionTemp->isChecked());
	ui.spinBoxSaturationLimit->setEnabled(ui.checkBoxOfflineCorrectionSat->isChecked());
	ui.doubleSpinBoxPDE->setEnabled(ui.checkBoxOfflineCorrectionSat->isChecked());
	ui.spinBoxNumPixels->setEnabled(ui.checkBoxOfflineCorrectionSat->isChecked());

	ui.spinBoxTOFPulseChannel->setEnabled(ui.checkBoxTimeOfFlight->isChecked());
	ui.comboBoxRelativePulse->setEnabled(ui.checkBoxTimeOfFlight->isChecked());
	ui.spinBoxStartGatePulse->setEnabled(ui.checkBoxTimeOfFlight->isChecked());
	ui.spinBoxPulseThreshold->setEnabled(ui.checkBoxTimeOfFlight->isChecked());
}

void AnalysisConfigDialog::codeChanged()
{    
	QPlainTextEdit* codeEdit = dynamic_cast<QPlainTextEdit*>(sender());
	if (codeEdit)
	{
		disconnect(codeEdit, SIGNAL(textChanged()), 0, 0);

		QTextCursor cursor(codeEdit->document());
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
		QTextCharFormat highlightFormat;
		highlightFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
        cursor.mergeCharFormat(highlightFormat);

        v8Mutex.lock();
        V8::Initialize();
        V8::InitializeICU();

        ArrayBufferAllocator allocator;
        Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = &allocator;
        isolate = Isolate::New(create_params);
        {
            Isolate::Scope isolate_scope(isolate);
            HandleScope handle_scope(isolate);


            Local<Context> context = Context::New(isolate);
            Context::Scope context_scope(context);

            v8::TryCatch try_catch;
            try_catch.SetVerbose(true);
            QString errorMessage;
            Local<String> source = String::NewFromUtf8(isolate, codeEdit->toPlainText().toStdString().c_str(),NewStringType::kNormal).ToLocalChecked();
            auto script = Script::Compile(context, source);
            if (script.IsEmpty())
                underlineError(codeEdit, getCompileErrorLine(try_catch, errorMessage)-1);
        }
        isolate->Dispose();
        v8Mutex.unlock();

		connect(codeEdit, SIGNAL(textChanged()), this, SLOT(codeChanged()), Qt::QueuedConnection);
	}	
}


void AnalysisConfigDialog::underlineError(QPlainTextEdit* textEdit, int lineNum)
{	
	QTextBlock block = textEdit->document()->findBlockByLineNumber(lineNum);	
	QTextCursor errorLine(block);
	errorLine.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

	QTextCharFormat highlightFormat;
	highlightFormat.setUnderlineColor(Qt::darkRed);
	highlightFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	errorLine.mergeCharFormat(highlightFormat);
}

#pragma endregion
