#include <iostream>
#include <QDateTime>
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	std::cout << "START " << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << std::endl;

	QApplication a(argc, argv);

	MainWindow w;
	w.show();

	return a.exec();
}
