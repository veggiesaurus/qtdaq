//#define VALGRIND

#ifdef VALGRIND
#include <vld.h> 
#endif
#include "qtdaq.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setOrganizationName("UCT Physics");
	a.setOrganizationDomain("phy.uct.ac.za");
	a.setApplicationName("QtDAQ");

	QtDAQ w;
	w.show();
	return a.exec();
}
