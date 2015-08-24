#pragma once

#include <QSyntaxHighlighter>

class VxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	VxHighlighter(QTextDocument *parent = 0);

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
	QTextCharFormat sectionFormat;
};