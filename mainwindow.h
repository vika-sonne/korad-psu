#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "ProtocolClass.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

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

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
