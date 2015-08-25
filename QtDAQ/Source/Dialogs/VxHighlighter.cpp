#include "VxHighlighter.h"

VxHighlighter::VxHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;
	keywordPatterns << "\\bOPEN\\b" << "\\bCORRECTION_LEVEL\\b" << "\\bOUTPUT_FILE_FORMAT\\b" << "\\bOUTPUT_FILE_HEADER\\b" << "\\bRECORD_LENGTH\\b" << "\\bTEST_PATTERN\\b"
		<< "\\bENABLE_DES_MODE\\b" << "\\bEXTERNAL_TRIGGER\\b" << "\\bFAST_TRIGGER\\b" << "\\bENABLED_FAST_TRIGGER_DIGITIZING\\b" << "\\bMAX_NUM_EVENTS_BLT\\b" << "\\bPOST_TRIGGER\\b"
		<< "\\bTRIGGER_EDGE\\b" << "\\bUSE_INTERRUPT\\b" << "\\bFPIO_LEVEL\\b" << "\\bWRITE_REGISTER\\b" << "\\bENABLE_INPUT\\b" << "\\bDC_OFFSET\\b"
		<< "\\bGRP_CH_DC_OFFSET\\b" << "\\bTRIGGER_THRESHOLD\\b" << "\\bCHANNEL_TRIGGER\\b" << "\\bGROUP_TRG_ENABLE_MASK\\b";

	foreach(const QString &pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	propertyFormat.setForeground(Qt::darkGray);
	propertyFormat.setFontWeight(QFont::Normal);
	QStringList propertyPatterns;
	propertyPatterns << "\\USB\\b" << "\\bPCI\\b" << "\\bPCIE\\b" << "\\bAUTO\\b" << "\\bASCII\\b" << "\\bBINARY\\b"
		<< "\\bYES\\b" << "\\bNO\\b" << "\\bDISABLED\\b" << "\\ACQUISITION_ONLY\\b" << "\\bACQUISITION_AND_TRGOUT\\b" << "\\bRISING\\b"
		<< "\\bFALLING\\b" << "\\bNIM\\b" << "\\bTTL\\b";

	foreach(const QString &pattern, propertyPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = propertyFormat;
		highlightingRules.append(rule);
	}

	QTextCharFormat valueFormat;
	valueFormat.setForeground(Qt::darkGray);
	rule.pattern = QRegExp("\\bYES\\b|\\bNO\\b|\\b[0-9]+\\b");
	rule.format = valueFormat;
	highlightingRules.append(rule);

	classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
	rule.format = classFormat;
	highlightingRules.append(rule);

	sectionFormat.setFontItalic(true);
	sectionFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("\\[\\S + \\]");
	rule.format = sectionFormat;
	highlightingRules.append(rule);

	singleLineCommentFormat.setForeground(Qt::red);
	rule.pattern = QRegExp("#[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

}

void VxHighlighter::highlightBlock(const QString &text)
{
	foreach(const HighlightingRule &rule, highlightingRules) {
		QRegExp expression(rule.pattern);
		int index = expression.indexIn(text);
		while (index >= 0) {
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = expression.indexIn(text, index + length);
		}
	}
	setCurrentBlockState(0);
}