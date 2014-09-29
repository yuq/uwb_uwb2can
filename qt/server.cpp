#include <QCoreApplication>
#include "uwb2can.h"

int server_main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    UWB2CAN uwb2can(&a);
    return a.exec();
}
