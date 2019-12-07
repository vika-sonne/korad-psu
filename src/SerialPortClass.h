#ifndef SerialPortClass_H
#define SerialPortClass_H

#include <QByteArray>
#include <QVector>
#include <QSerialPort>

class SerialPortClass : public QObject
{
	Q_OBJECT
public:
	typedef std::pair<int, int> VidPid;
	typedef std::pair<QString, QString> PortAndVidPid;

	//! @param vidPid	USB VID:PID list to search
	explicit SerialPortClass(QVector<VidPid> vidPid, QObject *parent=NULL);

public slots:
	//! Opens com port
	//! @param portName example: ttyACM0
	bool openSerialPort(PortAndVidPid portAndVidPid);
	void closeSerialPort(bool emitSignals);
	void closeSerialPortAndReconnect();

signals:
	void serialPortOpened(QString portName);
	void serialPortClosed(QString portName);

protected slots:
	void _serialPort_readyRead();
	void _serialPort_errorOccurred(QSerialPort::SerialPortError error);


protected:
	static constexpr int DEFAULT_BAUD = 9600;
	static constexpr QSerialPort::DataBits DEFAULT_DATA_BITS = QSerialPort::Data8;
	static constexpr QSerialPort::Parity DEFAULT_PARITY = QSerialPort::NoParity;
	static constexpr QSerialPort::StopBits DEFAULT_STOP_BITS = QSerialPort::OneStop;

	unsigned int _baud = DEFAULT_BAUD;
	QSerialPort::DataBits _dataBits = DEFAULT_DATA_BITS;
	QSerialPort::Parity _parity = DEFAULT_PARITY;
	QSerialPort::StopBits _stopBits = DEFAULT_STOP_BITS;

	QSerialPort *_serialPort = nullptr;	//!< Serial COM port

	QString _portName; //!< Port name to open
	QVector<VidPid> _vidPid; //!< USB VID:PID list to search

	int _reconnectTimerId = -1; //!< Com port find interval timer id: -1 - no timer; 0.. timer id
	int _reconnectAttemptsCount = 0; //!< Com port find attempts count: 0..
	QSerialPort::SerialPortError _lastError = QSerialPort::NoError; //! Error filter

	void timerEvent(QTimerEvent *event) override;

	virtual void portOpened() = 0;
	virtual void portClosed() = 0;
	virtual void dataArrived(QByteArray data) = 0;

	void processReconnectTimer();

	//! Tries to find not busy com port in the system
	//! @return Com port path (if found) or empty string
	PortAndVidPid tryFindComPort();

	void logData(QByteArray data, bool send=false);
};

#endif // SerialPortClass_H
