/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "document_cache.h"

#include "document.h"
#include "stack.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QTextStream>

#include <algorithm>

//-----------------------------------------------------------------------------

QString DocumentCache::m_path;

//-----------------------------------------------------------------------------

DocumentCache::DocumentCache(QObject* parent) :
	QObject(parent),
	m_ordering(0)
{
	QStringList entries = QDir(m_path).entryList(QDir::Files);
	if ((entries.count() >= 1) && entries.contains("mapping")) {
		m_previous_cache = backupCache();
	}
}

//-----------------------------------------------------------------------------

DocumentCache::~DocumentCache()
{
	backupCache();
}

//-----------------------------------------------------------------------------

bool DocumentCache::isClean() const
{
	return m_previous_cache.isEmpty();
}

//-----------------------------------------------------------------------------

bool DocumentCache::isWritable() const
{
	return QFileInfo(m_path).isWritable() && QFileInfo(m_path + "/../").isWritable();
}

//-----------------------------------------------------------------------------

void DocumentCache::parseMapping(QStringList& files, QStringList& datafiles) const
{
	QString cache_path = isClean() ? m_path : m_previous_cache;
	QFile file(cache_path + "/mapping");
	if (file.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream stream(&file);
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
		stream.setCodec("UTF-8");
#endif
		stream.setAutoDetectUnicode(true);

		while (!stream.atEnd()) {
			QString line = stream.readLine();
			QString datafile = line.section(' ', 0, 0);
			QString path = line.section(' ', 1);
			if (!datafile.isEmpty()) {
				files.append(path);
				datafiles.append(cache_path + "/" + datafile);
			}
		}
		file.close();
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::add(Document* document)
{
	m_filenames[document] = createFileName();
	connect(document, &Document::changedName, this, &DocumentCache::updateMapping);
	connect(document, &Document::replaceCacheFile, this, &DocumentCache::replaceCacheFile);
	connect(document, &Document::writeCacheFile, this, &DocumentCache::writeCacheFile);
	updateMapping();
}

//-----------------------------------------------------------------------------

void DocumentCache::remove(Document* document)
{
	if (m_filenames.contains(document)) {
		QString cache_file = m_filenames.take(document);
		updateMapping();
		QFile::remove(cache_file);
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::setOrdering(Stack* ordering)
{
	m_ordering = ordering;
}

//-----------------------------------------------------------------------------

void DocumentCache::setPath(const QString& path)
{
	m_path = path;
	if (!m_path.endsWith(QLatin1Char('/'))) {
		m_path += QLatin1Char('/');
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::updateMapping()
{
	QFile file(m_path + "/mapping");
	if (file.open(QFile::WriteOnly | QFile::Text)) {
		QTextStream stream(&file);
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
		stream.setCodec("UTF-8");
#endif
		stream.setGenerateByteOrderMark(true);

		for (int i = 0; i < m_ordering->count(); ++i) {
			Document* document = m_ordering->document(i);
			if (!m_filenames.contains(document)) {
				continue;
			}
			stream << QFileInfo(m_filenames[document]).baseName() << " " << document->filename()
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
				<< Qt::endl;
#else
				<< endl;
#endif
		}
		file.close();
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::replaceCacheFile(Document* document, const QString& file)
{
	QString cache_file = createFileName();
	if (QFile::copy(file, m_path + cache_file)) {
		updateCacheFile(document, cache_file);
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::writeCacheFile(Document* document, QSharedPointer<DocumentWriter> writer)
{
	QString cache_file = createFileName();
	writer->setFileName(m_path + cache_file);
	if (writer->write()) {
		updateCacheFile(document, cache_file);
	}
}

//-----------------------------------------------------------------------------

QString DocumentCache::backupCache()
{
	// Find backup location
	QString date = QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmss");
	int extra = 0;
	QDir dir(QDir::cleanPath(m_path + "/../"));
	QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware);
	subdirs.removeAll("Files");
	for (const QString& subdir : subdirs) {
		if (subdir.startsWith(date)) {
			extra = std::max(extra, subdir.mid(15).toInt() + 1);
		}
	}
	QString cachepath = dir.absoluteFilePath(date + ((extra == 0) ? "" : QString("-%1").arg(extra)));

	// Move cache files to backup location
	dir.rename("Files", cachepath);
	dir.mkdir("Files");

	// Limit to five backups
	while (subdirs.count() > 4) {
		QString subdir_name = subdirs.takeAt(0);
		QDir subdir(dir.absoluteFilePath(subdir_name));
		subdir.removeRecursively();
	}

	return cachepath;
}

//-----------------------------------------------------------------------------

QString DocumentCache::createFileName()
{
	QString filename;
	QDir dir(m_path);
	do {
		quint32 value = QRandomGenerator::global()->generate();
		filename = QString("fw_%1").arg(value, 6, 36, QLatin1Char('0'));
	} while (dir.exists(filename));

	return filename;
}

//-----------------------------------------------------------------------------

void DocumentCache::updateCacheFile(Document* document, const QString& cache_file)
{
	// Ensure new filenames only
	if (m_filenames[document] == cache_file) {
		return;
	}

	// Swap cache filename
	QFile old_cache_file(m_path + m_filenames[document]);
	m_filenames[document] = cache_file;
	updateMapping();

	// Delete old cache file
	if (old_cache_file.exists()) {
		old_cache_file.remove();
	}
}

//-----------------------------------------------------------------------------
