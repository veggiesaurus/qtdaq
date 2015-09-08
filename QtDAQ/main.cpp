//#define VALGRIND

#ifdef VALGRIND
#include <vld.h> 
#endif
#include "QtDAQ.h"
#include <QtWidgets/QApplication>
#include <include/v8.h>
#include <include/libplatform/libplatform.h>

int main(int argc, char *argv[])
{
    V8::InitializeICU();
    Platform* platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();

	QApplication a(argc, argv);
	a.setOrganizationName("UCT Physics");
	a.setOrganizationDomain("phy.uct.ac.za");
	a.setApplicationName("QtDAQ");

	QtDAQ w;
	w.show();
	return a.exec();


}
