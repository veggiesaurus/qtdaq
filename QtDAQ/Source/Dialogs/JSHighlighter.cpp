#include "JSHighlighter.h"

JSHighlighter::JSHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;
	keywordPatterns << "\\babstract\\b" << "\\barguments\\b" << "\\bboolean\\b" << "\\bbreak\\b" << "\\bbyte\\b" << "\\bcase\\b"
					<< "\\bcatch\\b" << "\\bchar\\b" << "\\bclass*\\b" << "\\bconst\\b" << "\\bcontinue\\b" << "\\bdebugger\\b"
					<< "\\bdefault\\b" << "\\bdelete\\b" << "\\bdo\\b" << "\\bdouble\\b" << "\\belse\\b" << "\\benum*\\b" 
					<< "\\beval\\b" << "\\bexport*\\b" << "\\bextends*\\b" << "\\bfinal\\b" << "\\bfinally\\b" 
					<< "\\bfloat\\b" << "\\bfor\\b" << "\\bfunction\\b" << "\\bgoto\\b" << "\\bif\\b" << "\\bimplements\\b" 
					<< "\\bimport*\\b" << "\\bin\\b" << "\\binstanceof\\b" << "\\bint\\b" << "\\binterface\\b" << "\\blet\\b" 
					<< "\\blong\\b" << "\\bnative\\b" << "\\bnew\\b" << "\\bnull\\b" << "\\bpackage\\b" << "\\bprivate\\b" 
					<< "\\bprotected\\b" << "\\bpublic\\b" << "\\breturn\\b" << "\\bshort\\b" << "\\bstatic\\b" << "\\bsuper*\\b" 
					<< "\\bswitch\\b" << "\\bsynchronized\\b" << "\\bthis\\b" << "\\bthrow\\b" << "\\bthrows\\b" << "\\btransient\\b" 
					<< "\\btry\\b" << "\\btypeof\\b" << "\\bvar\\b" << "\\bvoid\\b" << "\\bvolatile\\b"
					<< "\\bwhile\\b" << "\\bwith\\b" << "\\byield\\b";

	foreach(const QString &pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	propertyFormat.setForeground(Qt::blue);
	propertyFormat.setFontWeight(QFont::Normal);
	QStringList propertyPatterns;
	propertyPatterns << "\\bArray\\b" << "\\bchStats\\b" << "\\bDate\\b" << "\\beval\\b" << "\\bfunction\\b" << "\\bhasOwnProperty\\b" << "\\bInfinity\\b"
					<< "\\bisFinite\\b" << "\\bisNaN\\b" << "\\bisPrototypeOf\\b" << "\\blength\\b" << "\\bMath\\b" << "\\bNaN\\b"
					<< "\\bname\\b" << "\\bNumber\\b" << "\\bObject\\b" << "\\bprototype\\b" << "\\bString\\b" << "\\btoString\\b"
					<< "\\bundefined\\b" << "\\bvalueOf\\b";

	foreach(const QString &pattern, propertyPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = propertyFormat;
		highlightingRules.append(rule);
	}

	QTextCharFormat valueFormat;
	valueFormat.setForeground(Qt::darkGray);
	rule.pattern = QRegExp("\\btrue\\b|\\bfalse\\b|\\b[0-9]+\\b");
	rule.format = valueFormat;
	highlightingRules.append(rule);
	
	classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
	rule.format = classFormat;
	highlightingRules.append(rule);

	quotationSingleFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\".*\"");
	rule.format = quotationSingleFormat;
	highlightingRules.append(rule);

	quotationDoubleFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("'.*'");
	rule.format = quotationDoubleFormat;
	highlightingRules.append(rule);

	functionFormat.setFontItalic(true);
	functionFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = functionFormat;
	highlightingRules.append(rule);

	singleLineCommentFormat.setForeground(Qt::red);
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	multiLineCommentFormat.setForeground(Qt::red);

	commentStartExpression = QRegExp("/\\*");
	commentEndExpression = QRegExp("\\*/");
}

void JSHighlighter::highlightBlock(const QString &text)
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

	int startIndex = 0;
	if (previousBlockState() != 1)
		startIndex = commentStartExpression.indexIn(text);

	while (startIndex >= 0) {
		int endIndex = commentEndExpression.indexIn(text, startIndex);
		int commentLength;
		if (endIndex == -1) {
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		}
		else {
			commentLength = endIndex - startIndex
				+ commentEndExpression.matchedLength();
		}
		setFormat(startIndex, commentLength, multiLineCommentFormat);
		startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
	}
}