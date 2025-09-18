#include "mainwindow.h"
#include <QtWidgets> // 包含所有常用控件的头文件
#include "ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init_Data();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_Data(){

}