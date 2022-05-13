#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDateTime>

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
  connect(&m_client,&Client::disconnected,this,[&](){ setEnableButtons(false);
                                                     ui->textEdit->append("Disconnected!");
                                                     ui->textEdit->append("--------------------------"); });

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
  Packet packet;
  packet.setTypePacket(TypePacket::COMMAND);
  packet.appendMetaData(static_cast<char>(Command::SEND_AGAIN_XML_DATA));

  m_client.sendToServer(packet);
}

void MainWindow::fillForm(const Packet &packet)
{
  switch (packet.type()) {

  case TypePacket::MESSAGE:{

    ui->textEdit->append( QDateTime::currentDateTime().toString("HH:mm:ss") + ":");
    std::string str = "\0";
    str.insert(str.begin(),packet.getData().begin(),packet.getData().end());

    QStringList list = QString::fromUtf8(str.c_str()).split('\n'); // без этого не красит text в textEdit
    for(const QString& strLine : list){
       ui->textEdit->append(strLine);
    }

    ui->textEdit->append("--------------------------");
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


void MainWindow::on_pushButtonDisconnect_clicked()
{
  m_client.disconnect();
  setEnableButtons(false);
}

