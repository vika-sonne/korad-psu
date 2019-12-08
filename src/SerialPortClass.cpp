#include <iostream>
#include <math.h>
#include <QSerialPortInfo>
#include <QVector>
#include <QStringRef>
#include <QCommandLineParser>
#include "Log.h"
#include "SerialPortClass.h"

static constexpr int COM_PORT_FIND_START_INTERVAL = 1000; //!< Find inverval for low reconnect delay, ms
static constexpr int COM_PORT_FIND_START_ATTEMPTS = 5; //!< Attempts count for low reconnect delay
static constexpr int COM_PORT_FIND_INTERVAL = 1500; //!< ms

static QString _toEscapedCString(QByteArray buff)
{
	QString ret;
	foreach(auto ch, buff) {
		switch(ch)
		{
			case 0: ret += "\\0"; break;
			case 7: ret += "\\a"; break;
			case 8: ret += "\\b"; break;
			case 9: ret += "\\t"; break;
			case 0xa: ret += "\\n"; break;
			case 0xb: ret += "\\v"; break;
			case 0xc: ret += "\\f"; break;
			case 0xd: ret += "\\r"; break;
			default:
				if(ch < 0x20 || (uint8_t)ch > 0x7E)
				{
					QByteArray a(&ch, 1);
					ret += QString("\\x") + a.toHex().toUpper();
				}
				else
					ret += ch;
			break;
		}
	}
	return ret;
}

namespace _port
{
	inline QLatin1Char parity(QSerialPort *port)
	{
		switch(port->parity())
		{
			case QSerialPort::NoParity: return QLatin1Char('N');
			case QSerialPort::EvenParity: return QLatin1Char('E');
			case QSerialPort::OddParity: return QLatin1Char('O');
			case QSerialPort::SpaceParity: return QLatin1Char('S');
			case QSerialPort::MarkParity: return QLatin1Char('M');
			default: return QLatin1Char(' ');
		}
	}

	inline QString stopBits(QSerialPort *port)
	{
		switch(port->stopBits())
		{
			case QSerialPort::OneStop: return "1";
			case QSerialPort::OneAndHalfStop: return "1.5";
			case QSerialPort::TwoStop: return "2";
			default: return " ";
		}
	}

	inline QString parameters(QSerialPort *port)
	{
		auto ret = QString("%1 %2%3%4")
			.arg(port->baudRate())
			.arg(port->dataBits()).arg(parity(port)).arg(stopBits(port));
		return ret;
	}

	bool set_parameters(QString parameters,
		QSerialPort::DataBits *_dataBits,
		QSerialPort::Parity *_parity,
		QSerialPort::StopBits *_stopBits)
	{
		if(parameters.length() < 3)
			return false;

		// data bits
		switch(parameters[0].toLatin1())
		{
			case '5': *_dataBits = QSerialPort::Data5; break;
			case '6': *_dataBits = QSerialPort::Data6; break;
			case '7': *_dataBits = QSerialPort::Data7; break;
			case '8': *_dataBits = QSerialPort::Data8; break;
			default: return false;
		}

		// parity
		switch(parameters[1].toLatin1())
		{
			case 'N': *_parity = QSerialPort::NoParity; break;
			case 'E': *_parity = QSerialPort::EvenParity; break;
			case 'O': *_parity = QSerialPort::OddParity; break;
			case 'S': *_parity = QSerialPort::SpaceParity; break;
			case 'M': *_parity = QSerialPort::MarkParity; break;
			default: return false;
		}

		// stop bits
		switch(parameters[2].toLatin1())
		{
			case '2':
				if(parameters.length() != 3)
					return false;
				*_stopBits = QSerialPort::TwoStop;
				break;
			case '1':
				if(parameters.length() == 3)
					*_stopBits = QSerialPort::OneStop;
				else if(parameters.length() != 5 || parameters[3] != '.' || parameters[4] != '5')
					return false;
				else
					*_stopBits = QSerialPort::OneAndHalfStop;
				break;
			default: return false;
		}

		return true;
	}
}

SerialPortClass::SerialPortClass(QVector<VidPid> vidPid, QObject *parent)
	: QObject(parent), _vidPid(vidPid)
{
	// start reconnect timer // start to search port in the system by timer
	Log::msg(QString("T %0:%1").arg(__LINE__).arg(__FILE__));
	_reconnectTimerId = startTimer(0);
}

void SerialPortClass::processReconnectTimer()
{
	// com port not found
//	Log::msg(QString("T %0:%1").arg(__LINE__).arg(__FILE__));
	switch(_reconnectAttemptsCount)
	{
		case 0:
			// start reconnect timer with high rate
			_reconnectAttemptsCount = 1;
			killTimer(_reconnectTimerId);
			_reconnectTimerId = startTimer(COM_PORT_FIND_START_INTERVAL);
			break;
		case COM_PORT_FIND_START_ATTEMPTS:
			// start reconnect timer with normal rate
			_reconnectAttemptsCount++;
			killTimer(_reconnectTimerId);
			_reconnectTimerId = startTimer(COM_PORT_FIND_INTERVAL);
		case COM_PORT_FIND_START_ATTEMPTS + 1:
			// stop attempts counting
			break;
		default:
			// count: 1..COM_PORT_FIND_START_ATTEMPTS-1
			_reconnectAttemptsCount++;
			break;
	}
}

void SerialPortClass::timerEvent(QTimerEvent *event)
{
	if(event->timerId() == _reconnectTimerId)
	{
		auto portAndVidPid = tryFindComPort();
		if(!portAndVidPid.first.isEmpty())
		{
			// com port found
			if(openSerialPort(portAndVidPid))
			{
				// stop reconnect timer
				killTimer(_reconnectTimerId);
				_reconnectTimerId = -1;
				_reconnectAttemptsCount = 0;
			}
			else
				processReconnectTimer();
		}
		else
			processReconnectTimer();
	}
	else
	{
		killTimer(event->timerId());
		Log::msg("Timer not procced.");
	}
}

SerialPortClass::PortAndVidPid SerialPortClass::tryFindComPort()
{
	if(_vidPid.isEmpty())
		// not need to search com port since it specified
		return PortAndVidPid(_portName, _portName);

	// try to search com port according to VID:PID list
	foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
	{
		if(!info.isBusy() && info.hasVendorIdentifier() && info.hasProductIdentifier())
			foreach(VidPid vidPid, _vidPid)
			{
				if(info.vendorIdentifier() == vidPid.first
					&& info.productIdentifier() == vidPid.second)
					return PortAndVidPid(info.portName(),
						QString("found %0 USB %1:%2").arg(info.portName())
							.arg(info.vendorIdentifier(), 2, 16, QLatin1Char('0'))
							.arg(info.productIdentifier(), 2, 16, QLatin1Char('0'))
						);
			}
	}

	// port with USB VID:PID not found
	return PortAndVidPid(QString(), QString());
}

bool SerialPortClass::openSerialPort(SerialPortClass::PortAndVidPid portAndVidPid)
{
	// close serial port

	if(_serialPort != nullptr)
		_serialPort->close();

	// open serial port
	_serialPort = new QSerialPort(portAndVidPid.first, this);
	// setup serial port with parameters
	_serialPort->setBaudRate(_baud);
	_serialPort->setDataBits(_dataBits);
	_serialPort->setParity(_parity);
	_serialPort->setStopBits(_stopBits);
	// open serial port
	if(!_serialPort->open(QIODevice::ReadWrite))
	{
		if(_serialPort->error() != _lastError)
		{
			Log::error(QString("error  %0: (%1) %2").arg(portAndVidPid.first)
				.arg(_serialPort->error()).arg(_serialPort->errorString()));
			_lastError = _serialPort->error();
			if(_serialPort->isOpen())
				_serialPort->close();
			_serialPort = nullptr;
		}
		return false;
	}
	else
	{
		Log::error(QString("opened %0 @ %1").arg(portAndVidPid.second).arg(_port::parameters(_serialPort)));
	}

	// connect serial port signals
	connect(_serialPort, SIGNAL(readyRead()), this, SLOT(_serialPort_readyRead()));
	connect(_serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
		this, SLOT(_serialPort_errorOccurred(QSerialPort::SerialPortError)));

	portOpened();
	emit serialPortOpened(portAndVidPid.first);

	return true;
}

void SerialPortClass::closeSerialPort(bool emitSignals)
{
	if(_serialPort)
	{
		auto portName = _serialPort->portName();
		// filter
		_lastError = _serialPort->error();
		// log
		if(_serialPort->error() == QSerialPort::NoError)
			Log::error(QString("closed ") + qPrintable(portName));
		else
			Log::error(QString("closed %0: (%1) %2").arg(portName)
				.arg(_serialPort->error()).arg(_serialPort->errorString()));
		// close port
		if(_serialPort->isOpen())
			_serialPort->close();
		delete _serialPort;
		_serialPort = nullptr;

		if(emitSignals)
			emit serialPortClosed(portName);
	}

	if(emitSignals)
		portClosed();
}

void SerialPortClass::closeSerialPortAndReconnect()
{
	closeSerialPort(true);

	// start reconnect timer // start to search port in the system by timer
//	Log::msg(QString("T %0:%1").arg(__LINE__).arg(__FILE__));
	if(_reconnectTimerId >= 0)
		killTimer(_reconnectTimerId);
	_reconnectAttemptsCount = 0;
	_reconnectTimerId = startTimer(0);
}

void SerialPortClass::_serialPort_readyRead()
{
	auto buff = _serialPort->readAll();
	if(buff.length() == 0)
	{
		Log::msg("SERR2");
		if(_serialPort->error() != QSerialPort::NoError)
			closeSerialPortAndReconnect();
	}
	else
	{
		dataArrived(buff);
	}
}

void SerialPortClass::_serialPort_errorOccurred(QSerialPort::SerialPortError error)
{
	Log::msg(QString("SERR: %0").arg(error));
	if(error != QSerialPort::NoError)
		closeSerialPortAndReconnect();
}

void SerialPortClass::logData(QByteArray data, bool send)
{
	Log::data(_toEscapedCString(data), data.length(), send);
}
