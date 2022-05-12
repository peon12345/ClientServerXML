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
  ui->lineEditName->setText("Игорь");

  ui->imageView->setAlignment(Qt::AlignCenter);
  setEnableButtons(false);

  connect(&m_client,&Client::packageReceived,this,&MainWindow::fillForm,Qt::BlockingQueuedConnection);
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
  if(ui->lineEditName->text().isEmpty()){
    ui->textEdit->setText("Enter your name");
    return void();
  }else{
    m_client.setName(ui->lineEditName->text());
  }

  try{
    m_client.connectToServer(ui->lineEditIP->text().toStdString(),ui->lineEditPort->text().toStdString());
    m_client.listenServer();
    setEnableButtons(true);


  }catch(const char* str){
    ui->textEdit->setText(str);
  }
}


void MainWindow::on_pushButtonRefresh_clicked()
{

}

void MainWindow::fillForm(const Packet &packet)
{
  switch (packet.type()) {

  case TypePacket::MESSAGE:{
    std::string str ( packet.getData().begin(),packet.getData().end() );
    str += '\0';
    ui->textEdit->setText(QString::fromLocal8Bit( str.c_str()));
    break;
  }

  case TypePacket::IMAGE:{
    QPixmap pix;

    const char* format = packet.getMetaData().data();
    pix.loadFromData(QByteArray::fromBase64(packet.getData().data()),format);
    ui->imageView->setPixmap(pix);
  }


  default:

    break;

  }
}















