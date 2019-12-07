#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QPen>
#include <QTime>
#include "ProtocolClass.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

protected:
	class GraphParametersClass
	{
	public:
		QPen u() const { return QPen(QBrush(Qt::blue), 2); }
		QPen i() const { return QPen(QBrush(Qt::red), 2); }
		uint timeDept() const { return 60; } // seconds
		uint timeTick() const { return 10; } // seconds
	};

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

signals:
	void request(ProtocolClass::RequestEnum r);
	void stop();

protected slots:
	void _protocol_serialPortOpened(QString portName);
	void _protocol_serialPortClosed(QString portName);
	void _protocol_modelDetected(QString model);
	void _protocol_answer(ProtocolClass::RequestEnum r, QByteArray value);
	void _protocol_answerTimeout();

protected:
	ProtocolClass _protocol;
	QThread _protocolThread;
	QString _portName;
	GraphParametersClass _graphParameters;
	QTime _time;

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
