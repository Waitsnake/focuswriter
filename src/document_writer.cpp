/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "document_writer.h"

#include "docx_writer.h"
#include "odt_writer.h"
#include "rtf_writer.h"

#include <QFile>
#include <QTextDocument>
#include <QTextStream>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#include <errno.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <io.h>
#endif

//-----------------------------------------------------------------------------

DocumentWriter::DocumentWriter() :
	m_type("fodt"),
	m_document(0),
	m_write_bom(false)
{
}

//-----------------------------------------------------------------------------

DocumentWriter::~DocumentWriter()
{
	if (m_document && !m_document->parent()) {
		delete m_document;
	}
}

//-----------------------------------------------------------------------------

bool DocumentWriter::write()
{
	Q_ASSERT(m_document != 0);
	Q_ASSERT(!m_filename.isEmpty());

	bool saved = false;

	QFile file(m_filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}

	if (m_type == "odt") {
		OdtWriter writer;
		saved = writer.write(&file, m_document);
	} else if (m_type == "fodt") {
		OdtWriter writer;
		writer.setFlatXML(true);
		saved = writer.write(&file, m_document);
	} else if (m_type == "docx") {
		DocxWriter writer;
		saved = writer.write(&file, m_document);
	} else if (m_type == "rtf") {
		file.setTextModeEnabled(true);
		RtfWriter writer(m_encoding);
		if (m_encoding.isEmpty()) {
			m_encoding = writer.encoding();
		}
		saved = writer.write(&file, m_document);
	} else {
		file.setTextModeEnabled(true);
		QTextStream stream(&file);
		QByteArray encoding = !m_encoding.isEmpty() ? m_encoding : "UTF-8";
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
		if (auto e = QStringConverter::encodingForName(encoding)) {
			stream.setEncoding(*e);
		}
#else
		stream.setCodec(encoding);
#endif
		if (m_write_bom || (encoding != "UTF-8")) {
			stream.setGenerateByteOrderMark(true);
		}
		stream << m_document->toPlainText();
		saved = stream.status() == QTextStream::Ok;
	}

	saved &= file.flush();
#if defined(Q_OS_UNIX)
	int ret;
	do {
		ret = fsync(file.handle());
	} while ((ret == -1) && (errno == EINTR));
	saved &= (ret == 0);
#elif defined(Q_OS_WIN)
	saved &= (FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(file.handle()))) != 0);
#endif
	file.close();

	return saved;
}

//-----------------------------------------------------------------------------
