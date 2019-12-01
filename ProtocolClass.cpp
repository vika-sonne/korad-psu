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

void ProtocolClass::timerEvent(QTimerEvent *event)
{
	if(_timerId == event->timerId())
	{
		if(_serialPort && _serialPort->isOpen())
		{
			killTimer(_timerId);
			_timerId = -1;
			if(_rxBuff.isEmpty())
			{
				// RX the first byte // wait for answer RX complete
				Log::msg(QString("T %0:%1").arg(__LINE__).arg(__FILE__));
				_timerId = startTimer(DEFAULT_ANSWER_INTERBYTE_TIMEOUT);
			}
			else
			{
				// answer RX complete
				logData(_rxBuff);

				auto r = _request;
				auto buff = _rxBuff;
				_request = RequestEnum::None;
				_rxBuff.clear();
				_serialPort->clear();
				emit answer(r, buff);

				if(r == RequestEnum::IDN)
				{
					_modelIsOk = _parseIdn(buff);
					if(_modelIsOk)
						emit modelDetected(buff);
				}
			}
		}
		else
		{
			// serial port was closed
			killTimer(_timerId);
			_timerId = -1;
		}
	}
	else
		SerialPortClass::timerEvent(event);
}

void ProtocolClass::portOpened()
{
	_modelIsOk = false;
	_rxBuff.clear();
	_request = RequestEnum::None;
	if(_timerId >= 0)
	{
		killTimer(_timerId);
		_timerId = -1;
	}

	QThread::msleep(500);

	request(RequestEnum::IDN);
}

void ProtocolClass::portClosed()
{
	_modelIsOk = false;
	_rxBuff.clear();
	_request = RequestEnum::None;
	if(_timerId >= 0)
	{
		killTimer(_timerId);
		_timerId = -1;
	}
}

void ProtocolClass::dataArrived(QByteArray data)
{
	_rxBuff += data;
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
			case RequestEnum::IDN: sendRequest("*IDN?", r, true); break;
			case RequestEnum::STATUSQ: sendRequest("STATUS?", r, true); break;
			case RequestEnum::VSET1Q: sendRequest("VSET1?", r, true); break;
			case RequestEnum::VSET1: sendRequest(QString("VSET1:%0").arg(value).toLatin1(), r); break;
			case RequestEnum::VOUT1Q: sendRequest("VOUT1?", r, true); break;
			case RequestEnum::ISET1Q: sendRequest("ISET1?", r, true); break;
			case RequestEnum::IOUT1Q: sendRequest("IOUT1?", r, true); break;
			default:
				return;
		}
	}
}

void ProtocolClass::sendRequest(QByteArray data, RequestEnum requested, bool waitAnswer)
{
	if(_serialPort && _serialPort->isOpen())
	{
		logData(data, true);
		_rxBuff.clear();
		if(waitAnswer)
		{
			// wait for answer // start to RX with timeout
			_request = requested;
			_serialPort->clear();
			_serialPort->write(data);
			if(_timerId >= 0)
				killTimer(_timerId);
			Log::msg(QString("T %0:%1").arg(__LINE__).arg(__FILE__));
			_timerId = startTimer(requested == RequestEnum::IDN ?  DEFAULT_IDN_ANSWER_TIMEOUT : DEFAULT_ANSWER_TIMEOUT);
		}
		else
		{
			// ready for next request
			if(!_serialPort->waitForBytesWritten(DEFAULT_REQUEST_SEND_TIMEOUT))
				Log::msg("Request TX timeout");
			_request = RequestEnum::None;
			if(_serialPort && _serialPort->isOpen())
				_serialPort->clear();
			emit answer(requested, QByteArray());
		}
	}
	else
	{
		Log::msg("Try write to closed port");
	}
}
