#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <float.h>
#include <math.h>
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

	class AutoscaleClass
	{
	public:
		AutoscaleClass(float max) : max(max), maxValue(0.) {}
		float max; //!< Maximum value to scale down
		float maxValue; //!< Current maximum value
		bool scaleMax(float value)
		{
			if(value <= DBL_EPSILON)
				return false;
			float newMax = 2 * (int)log2(max / value);
			newMax = newMax > DBL_EPSILON ? max / newMax : max;
			if(fabs(newMax - maxValue) > max * 0.001)
			{
				maxValue = newMax;
				return true;
			}
			return false;
		}
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
	AutoscaleClass _u_autoscale;
	AutoscaleClass _i_autoscale;

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
