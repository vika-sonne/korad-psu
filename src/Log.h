#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <QString>
#include <QDateTime>

class Log
{
public:
	static inline void error(QString msg)
	{
		std::cout << qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz ")) << '<' << qPrintable(msg) << '>' << std::endl;
	}

	static void msg(QString msg)
	{
		std::cout << qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz ")) << qPrintable(msg) << std::endl;
	}

	static void data(QString data, uint len, bool send=false, bool printTimestamp=true, bool printLen=true)
	{
		std::cout << (printTimestamp ? qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz ")) : "             ")
			<< (printLen ? qPrintable(QString("%0 %1 ").arg(len, 2, 10, QLatin1Char('0')).arg(send ? ">>" : "<<")) : "      ")
			<< qPrintable(data)
			<< std::endl;
	}
};

#endif // LOG_H
