#include <QMetaEnum>
#include <QFontDatabase>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Log.h"

#include <QThread>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), _protocol(),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	Log::msg(QString("MAIN %0").arg((ulong)QThread::currentThreadId(), 0, 16));
	_protocol.moveToThread(&_protocolThread);
	_protocolThread.start();

	QFontDatabase fontDB;
	fontDB.addApplicationFont(":/font/Digital-7.ttf");

	connect(&_protocol, SIGNAL(serialPortOpened(QString)), SLOT(_protocol_serialPortOpened(QString)));
	connect(&_protocol, SIGNAL(serialPortClosed(QString)), SLOT(_protocol_serialPortClosed(QString)));
	connect(&_protocol, SIGNAL(modelDetected(QString)), SLOT(_protocol_modelDetected(QString)));
	connect(&_protocol, SIGNAL(answerTimeout()), SLOT(_protocol_answerTimeout()));
	connect(&_protocol, SIGNAL(answer(ProtocolClass::RequestEnum,QByteArray)),
		SLOT(_protocol_answer(ProtocolClass::RequestEnum,QByteArray)));

	connect(this, SIGNAL(request(ProtocolClass::RequestEnum)), &_protocol, SLOT(request(ProtocolClass::RequestEnum)));
	connect(this, SIGNAL(stop()), &_protocol, SLOT(stop()));

	// setup the graph
	{
		auto plot = ui->oGraph;
		// add graphs
		{
			auto graph = plot->addGraph(plot->xAxis, plot->yAxis);
			graph->setPen(_graphParameters.u());
			graph->setName("V");
		}
		{
			auto graph = plot->addGraph(plot->xAxis, plot->yAxis2);
			graph->setPen(_graphParameters.i());
			graph->setName("A");
		}
		// add time axies
		{
			QSharedPointer<QCPAxisTickerFixed> timeTicker(new QCPAxisTickerFixed);
//			timeTicker->setTimeFormat("%h:%m:%s");
			timeTicker->setTickStep(_graphParameters.timeTick());
			plot->xAxis->setTicker(timeTicker);
			plot->xAxis->setTicks(false);
		}
		plot->yAxis->setRange(0, 31.5);
		plot->yAxis->setLabel("V");
		plot->yAxis->setVisible(true);
		plot->yAxis->setTickLabelColor(_graphParameters.u().color());

		plot->yAxis2->setRange(0, 3.15);
		plot->yAxis2->setLabel("A");
		plot->yAxis2->setVisible(true);
		plot->yAxis2->setTickLabelColor(_graphParameters.i().color());
	}
}

MainWindow::~MainWindow()
{
	delete ui;

	Log::msg("Wait for serial thread...");
	emit stop();
	_protocolThread.quit();
	_protocolThread.wait(500);
	Log::msg("Done");
}

void MainWindow::_protocol_serialPortOpened(QString portName)
{
	_portName = portName;
	ui->oStatusBar->showMessage(QString("Port: ") + portName);
}

void MainWindow::_protocol_serialPortClosed(QString portName)
{
	_portName = portName;
	ui->oStatusBar->showMessage(QString("Port: ") + portName + " closed");
	ui->oVset1->setText("--.--");
	ui->oVout->setText("--.--");
	ui->oIset1->setText("-.---");
	ui->oIout->setText("-.---");
}

void MainWindow::_protocol_modelDetected(QString model)
{
	ui->oStatusBar->showMessage(QString("Port: ") + _portName + "; Model: " + model);

	_time = QTime::currentTime();

	// start the polling cycle
	emit request(ProtocolClass::RequestEnum::VSET1Q);
}

void MainWindow::_protocol_answer(ProtocolClass::RequestEnum r, QByteArray value)
{
//	Log::msg(QString("%0 %1").arg((int)request).arg(QString(value)));
	double key = _time.elapsed() / 1000.0; // seconds
	auto plot = ui->oGraph;
	switch(r)
	{
		case ProtocolClass::RequestEnum::VSET1Q:
			ui->oVset1->setText(value);
			emit request(ProtocolClass::RequestEnum::ISET1Q);
			break;
		case ProtocolClass::RequestEnum::ISET1Q:
			ui->oIset1->setText(value);
			plot->xAxis->setRange(key, _graphParameters.timeDept(), Qt::AlignRight);
			plot->replot();
			emit request(ProtocolClass::RequestEnum::VOUT1Q);
			break;
		case ProtocolClass::RequestEnum::VOUT1Q:
			ui->oVout->setText(value);
			plot->graph(0)->addData(key, QString(value).toFloat());
			plot->xAxis->setRange(key, _graphParameters.timeDept(), Qt::AlignRight);
			plot->replot();
			emit request(ProtocolClass::RequestEnum::IOUT1Q);
			break;
		case ProtocolClass::RequestEnum::IOUT1Q:
			ui->oIout->setText(value);
			plot->graph(1)->addData(key, QString(value).toFloat());
			emit request(ProtocolClass::RequestEnum::VSET1Q);
			break;
		default: break;
	}
}

void MainWindow::_protocol_answerTimeout()
{
}
