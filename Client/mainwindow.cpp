#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  ui->lineEditIP->setText("127.0.0.1");
  ui->lineEditPort->setText("8005");

  ui->imageView->setAlignment(Qt::AlignCenter);
  setEnableButtons(false);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::setEnableButtons(bool isEnable)
{
  ui->pushButtonRefresh->setEnabled(isEnable);
  ui->pushButtonDisconnect->setEnabled(isEnable);


  ui->lineEditIP->setEnabled(!isEnable);
  ui->lineEditPort->setEnabled(!isEnable);
  ui->lineEditName->setEnabled(!isEnable);

  ui->pushButtonConnect->setEnabled(!isEnable);
}




void MainWindow::on_pushButtonConnect_clicked()
{
  //try{
    m_client.connectToServer(ui->lineEditIP->text().toStdString(),ui->lineEditPort->text().toStdString());
    m_client.listenServer();
    setEnableButtons(true);

//  }catch(const QString& str){

//    ui->textEdit->setText(str);
//  }
}

