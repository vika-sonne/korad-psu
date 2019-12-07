#ifndef ProtocolClass_H
#define ProtocolClass_H

#include <QByteArray>
#include <QVector>
#include <QThread>
#include "SerialPortClass.h"

class ProtocolClass : public SerialPortClass
{
	Q_OBJECT

public:
	enum class RequestEnum
	{
		None,

		IDN, //!< Request identification from device
		STATUSQ, //!< Request the actual status

		VSET1Q, //!< Request the voltage as set by the user
		VSET1, //!< Set the maximum output voltage
		VOUT1Q, //!< Request the actual voltage output

		ISET1Q, //!< Request the current as set by the user
		ISET1, //!< Set the maximum output current
		IOUT1Q, //!< Request the actual output current

		OUT0, //!< Disable the power output
		OUT1, //!< Enable the power output
		OVP0, //!< Disable the "Over Voltage Protection"
		OVP1, //!< Enable the "Over Voltage Protection"
		OCP0, //!< Disable the "Over Current Protection"
		OCP1, //!< Enable the "Over Current Protection"

		RCL1, //!< Recalls voltage and current limits from memory: 1..5
		SAV1, //!< Saves voltage and current limits to memory: 1..5

		TRACK0, //!< Set multichannel mode, 0 independent, 1 series, 2 parallel
	};
	Q_ENUM(RequestEnum)

	explicit ProtocolClass(QObject *parent=NULL);

public slots:
	void request(ProtocolClass::RequestEnum r);
	void request(ProtocolClass::RequestEnum r, float value);

	//! Stops & cleanups the protocol state & timers
	void stop();

signals:
	void answer(ProtocolClass::RequestEnum request, QByteArray value);
	void answerTimeout();

	//! IDN answer
	//! @param model	Answer: model, version, S/N, example: "KORAD KA3005P V4.2 SN:########", where # - digit
	void modelDetected(QString model);

protected:
	static constexpr int DEFAULT_IDN_ANSWER_TIMEOUT = 250; //!< Answer timeout, ms
	static constexpr int DEFAULT_ANSWER_TIMEOUT = 150; //!< Answer timeout, ms
	static constexpr int DEFAULT_OPEN_PORT_DELAY = 500; //!< Delay after port opened, ms

	bool _modelIsOk = false; //!< IDN answer parsing result

	RequestEnum _request = RequestEnum::None; //!< Request
	QByteArray _rxBuff; //!< Answer buffer
	int _timerId = -1; //!< Answer timeout timer ID: -1 - timer not launched; 0..
	int _answerExpectedLen = 0; //!< Answer expected length, bytes: 0..

	void timerEvent(QTimerEvent *event) override;

	void portOpened() override;
	void portClosed() override;

	void dataArrived(QByteArray data) override;

	void sendRequest(QByteArray data, RequestEnum r, int answerExpectedLen=0);

	//! Completes answer recieving
	virtual void requestComplete();

	//! Completes answer recieving
	virtual void clear();
};

#endif // ProtocolClass_H
