#include "../include/mainwindow.h"
#include <QApplication>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
int main(int argc, char *argv[])
{
    //QApplication a(argc, argv);
    //MainWindow w;
    //w.show();
    //return a.exec();
    string Model = "PEModel";
    PE_data PEdata;
    Propagation_Engine PE(Model);
    PE.PEmodel_computing2D(PEdata, 25);

}
