/*
	SPDX-FileCopyrightText: 2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef HTML_WRITER_H
#define HTML_WRITER_H

#include <QCoreApplication>
#include <QXmlStreamWriter>
class QTextDocument;

class HtmlWriter
{
	Q_DECLARE_TR_FUNCTIONS(HtmlWriter)

public:
	QString errorString() const
	{
		return m_error;
	}

	bool write(QIODevice* device, QTextDocument* document);

private:
	QXmlStreamWriter m_xml;
	QString m_error;
};

#endif
