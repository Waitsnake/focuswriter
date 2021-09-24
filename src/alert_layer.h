/*
	SPDX-FileCopyrightText: 2010-2012 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef ALERT_LAYER_H
#define ALERT_LAYER_H

class Alert;

#include <QList>
#include <QMessageBox>
#include <QWidget>
class QVBoxLayout;

class AlertLayer : public QWidget
{
	Q_OBJECT

public:
	AlertLayer(QWidget* parent);

public slots:
	void addAlert(Alert* alert);

private slots:
	void alertDestroyed(QObject* alert);
	void dismissAlert();

private:
	QList<Alert*> m_alerts;
	QVBoxLayout* m_alerts_layout;
};

#endif
