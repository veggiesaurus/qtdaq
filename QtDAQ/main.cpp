//#define VALGRIND

#ifdef VALGRIND
#include <vld.h> 
#endif
#include "QtDAQ.h"
#include <QtWidgets/QApplication>
#include <include/v8.h>

int main(int argc, char *argv[])
{

    auto plat = platform::CreateDefaultPlatform();
    V8::InitializePlatform(plat);

    QApplication a(argc, argv);
	a.setOrganizationName("UCT Physics");
	a.setOrganizationDomain("phy.uct.ac.za");
	a.setApplicationName("QtDAQ");

	QtDAQ w;
	w.show();
	return a.exec();


}
