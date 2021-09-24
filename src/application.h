/*
	SPDX-FileCopyrightText: 2011-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef APPLICATION_H
#define APPLICATION_H

class Window;

#include <QStringList>
#include <QtSingleApplication>

class Application : public QtSingleApplication
{
public:
	Application(int& argc, char** argv);

	QStringList files() const
		{ return m_files; }

	bool createWindow(const QStringList& files);

protected:
	virtual bool event(QEvent* e);

private:
	QStringList m_files;
	Window* m_window;
};

#endif
