#include <iostream>
#include <math.h>
#include <QDateTime>
#include <QSerialPortInfo>
#include <QVector>
#include <QStringRef>
#include <QCommandLineParser>
#include "Log.h"
#include "ProtocolClass.h"

#include <QThread>

bool _parseIdn(QByteArray answer)
{
	// "KORAD KA3005P V4.2 SN:########", where # - digit
	if(answer.startsWith("KORAD"))
	{
		if(answer.mid(5).trimmed().startsWith("KA3005P"))
			return true;
	}
	return false;
}

ProtocolClass::ProtocolClass(QObject *parent) :
	SerialPortClass({ VidPid(0x0416, 0x5011) }, parent)
{
	qRegisterMetaType<ProtocolClass::RequestEnum>();
	std::cout << "START " << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << std::endl;
}

void ProtocolClass::clear()
{
	_request = RequestEnum::None;
	_answerExpectedLen = 0;
	_rxBuff.clear();
	if(_serialPort && _serialPort->isOpen())
		_serialPort->clear();
	if(_timerId >= 0)
	{
		killTimer(_timerId);
		_timerId = -1;
	}
}

void ProtocolClass::requestComplete()
{
	// answer RX complete
	logData(_rxBuff);

	auto r = _request;
	auto buff = _rxBuff;
	clear();
	emit answer(r, buff);

	if(r == RequestEnum::IDN)
	{
		// parse IDN answer for PSU model e.t.c.
		_modelIsOk = _parseIdn(buff);
		if(_modelIsOk)
			emit modelDetected(buff);
		else
			// unknown model or broken connection
			closeSerialPort();
	}
}

void ProtocolClass::timerEvent(QTimerEvent *event)
{
	if(_timerId == event->timerId())
	{
		if(_serialPort && _serialPort->isOpen())
		{
			if(_request == RequestEnum::IDN)
				// since, IDN answer length is not known - use timeout to RX the answer
				requestComplete();
			else
			{
				// request timeout detected
				Log::msg("Timeout");
				clear();
				emit answerTimeout();
			}
		}
		else
			// serial port was closed
			clear();
	}
	else
		SerialPortClass::timerEvent(event);
}

void ProtocolClass::portOpened()
{
	_modelIsOk = false;
	clear();

	QThread::msleep(500);
	request(RequestEnum::IDN);
}

void ProtocolClass::portClosed()
{
	_modelIsOk = false;
	clear();
}

void ProtocolClass::dataArrived(QByteArray data)
{
	_rxBuff += data;

	// check RX buffer for expected answer
	if(_request != RequestEnum::IDN)
		if(_rxBuff.length() >= _answerExpectedLen)
			// answer RX complete
			requestComplete();
}

void ProtocolClass::request(RequestEnum r)
{
	request(r, 0.0);
}

void ProtocolClass::request(RequestEnum r, float value)
{
	if(!_modelIsOk && r != RequestEnum::IDN)
		return;

	if(_request == RequestEnum::None || r != _request)
	{
		switch(r)
		{
			case RequestEnum::IDN: sendRequest("*IDN?", r, 1024); break;
			case RequestEnum::STATUSQ: sendRequest("STATUS?", r, 1); break;
			case RequestEnum::VSET1Q: sendRequest("VSET1?", r, 5); break;
			case RequestEnum::VSET1: sendRequest(QString("VSET1:%0").arg(value).toLatin1(), r); break;
			case RequestEnum::VOUT1Q: sendRequest("VOUT1?", r, 5); break;
			case RequestEnum::ISET1Q: sendRequest("ISET1?", r, 5); break;
			case RequestEnum::IOUT1Q: sendRequest("IOUT1?", r, 5); break;
			default:
				return;
		}
	}
}

void ProtocolClass::sendRequest(QByteArray data, RequestEnum r, int answerExpectedLen)
{
	if(_serialPort && _serialPort->isOpen())
	{
		logData(data, true);
		// flush serial port data with timeout
		if(_serialPort && _serialPort->isOpen())
			if(!_serialPort->waitForBytesWritten(DEFAULT_REQUEST_SEND_TIMEOUT))
				Log::msg("Request TX timeout");
		_rxBuff.clear();
		// send request
		if(answerExpectedLen > 0)
		{
			// send request & wait for answer with timeout
			_request = r;
			_answerExpectedLen = answerExpectedLen;
			if(_serialPort && _serialPort->isOpen())
			{
				_serialPort->clear();
				_serialPort->write(data);
			}
			// wait for answer with timeout
			if(_timerId >= 0)
				killTimer(_timerId);
			Log::msg(QString("T %0:%1").arg(__LINE__).arg(__FILE__));
			_timerId = startTimer(r == RequestEnum::IDN ? DEFAULT_IDN_ANSWER_TIMEOUT : DEFAULT_ANSWER_TIMEOUT);
		}
		else
		{
			// send request
			_request = RequestEnum::None;
			_answerExpectedLen = 0;
			if(_serialPort && _serialPort->isOpen())
			{
				_serialPort->clear();
				_serialPort->write(data);
			}
			// ready for next request
			emit answer(r, QByteArray());
		}
	}
	else
	{
		Log::msg("Try write to closed port");
	}
}
