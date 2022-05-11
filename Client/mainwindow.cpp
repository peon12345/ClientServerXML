#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  ui->ipLine->setText("127.0.0.1");
  ui->portLine->setText("1112");

}

MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::on_connectButton_clicked()
{
  try{
    m_client.connectToServer(ui->ipLine->text().toLocal8Bit().constData(),ui->portLine->text().toLocal8Bit().constData());
    m_client.listenServer();
  }

  catch(const char* exeption){

    qDebug() << exeption;
    return void();
  }
}


void MainWindow::on_disconnectButton_clicked()
{

}


void MainWindow::on_pushButton_2_clicked()
{
  QByteArray ba = ui->messageLine_2->text().toLocal8Bit();

  std::vector<char> data(Data::sizeHeader(TypePacket::DATA));
  data.insert(data.end(),ba.begin(),ba.end());

  if(!m_client.sendToServer(data,TypePacket::DATA,true)){
    qDebug() << "SEND ERROR";
  }

}

