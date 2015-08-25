#pragma once

#include <QSyntaxHighlighter>

class JSHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	JSHighlighter(QTextDocument *parent = 0);

protected:
	void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
	struct HighlightingRule
	{
		QRegExp pattern;
		QTextCharFormat format;
	};
	QVector<HighlightingRule> highlightingRules;

	QRegExp commentStartExpression;
	QRegExp commentEndExpression;

	QTextCharFormat keywordFormat;
	QTextCharFormat propertyFormat;
	QTextCharFormat classFormat;
	QTextCharFormat singleLineCommentFormat;
	QTextCharFormat multiLineCommentFormat;
	QTextCharFormat quotationSingleFormat;
	QTextCharFormat quotationDoubleFormat;
	QTextCharFormat functionFormat;
};