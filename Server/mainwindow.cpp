#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDomDocument>
#include "xmlparserformat1.h"
#include "xmldata.h"
#include <QPixmap>
#include <QMessageBox>
#include <QAbstractItemView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->lineEditIP->setText("127.0.0.1");
  ui->lineEditPort->setText("8005");

  ui->imageView->setAlignment(Qt::AlignCenter);
  setEnableButtons(false);


  m_modelNewConnections = new QStringListModel(this);
  ui->listViewConnectedClients->setModel(m_modelNewConnections);
  ui->listViewConnectedClients->setEditTriggers(QAbstractItemView::NoEditTriggers);



  connect(&m_server,&Server::clientConnected,this,&MainWindow::addNewClient);
}

MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::on_pushButtonStart_clicked()
{

  try{

    m_server.initServer(ui->lineEditIP->text().toStdString(),ui->lineEditPort->text().toInt());

  }catch(const QString& str){
    QMessageBox msg;
    msg.setInformativeText("Error start server!");
    msg.setText(str);
    msg.setIcon(QMessageBox::Critical);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.exec();
    return void();
  }

  ui->textEdit->setText("Server started!");

  m_server.startListen();

  ui->textEdit->setText("The server is waiting for clients to connect...");

  setEnableButtons(true);
}


void MainWindow::on_pushButtonDisconnectClient_clicked()
{
  QModelIndex index = ui->listViewConnectedClients->currentIndex();
  QString itemText = index.data(Qt::DisplayRole).toString();
  m_server.disconnectClientByName(itemText);
  m_modelNewConnections->removeRow(index.row());
}


void MainWindow::on_pushButtonLoadXml_clicked()
{
  QString pathXml  =  QFileDialog::getOpenFileName(0, "Open Dialog", "", "*.xml");
  if(pathXml.isEmpty()){
    return void();
  }

  std::optional<QDomDocument> xmlDoc = XmlReader::read(pathXml);

  if(!xmlDoc){
    ui->textEdit->setText("Read error!");
    return void();
  }

  xmlHandler(xmlDoc.value());
}

void MainWindow::addNewClient(const QString &name)
{
  if(m_modelNewConnections){
  if(m_modelNewConnections->insertRow(m_modelNewConnections->rowCount())) {
      QModelIndex index = m_modelNewConnections->index(m_modelNewConnections->rowCount() - 1, 0);
      m_modelNewConnections->setData(index, name);
  }
    }

}

void MainWindow::xmlHandler(const QDomDocument &xmlDoc)
{
  //в будущем здесь можно узнать тип документа
  XmlParser::XmlFormatSupport format = XmlParser::XmlFormatSupport::MESSAGE_WITH_IMAGE;

  std::unique_ptr<XmlParser> xmlParser;
  std::vector<QString> attributesToSend;

  switch (format) {

  case XmlParser::XmlFormatSupport::MESSAGE_WITH_IMAGE:{

    xmlParser = std::make_unique<XmlParserFormat1>();

    static const std::vector<QString> sendNameAttributes{"from","color","text","image"};
    attributesToSend = sendNameAttributes;

    break;
  }


  //...
  //другие XML форматы

  default:

    ui->textEdit->setText("this XML is not supported!");
    return void();
  }

  XmlData xmlData;

  try{
    xmlData = xmlParser->parse(xmlDoc);
  }catch(...){
    return void();
  }

  fillForm(xmlData);
  sendXmlData(xmlData.values(attributesToSend),xmlData.values("to"));

}

void MainWindow::fillForm(const XmlData &xmlData)
{

  ui->textEdit->clear();

  std::list<QByteArray> image = xmlData.values("image");

  if(!image.empty()){
    QPixmap pix;
    pix.loadFromData(QByteArray::fromBase64(image.front()),"bmp");
    ui->imageView->setPixmap(pix);
  }

  ui->textEdit->append("FormatVersion:" + xmlData.values("FormatVersion").front() + " " + "msg id:" + xmlData.values("id").front() );
  ui->textEdit->append("From:" + xmlData.values("from").front() + " | " + "To:" + xmlData.values("to").front());
  ui->textEdit->append("Text:");
  ui->textEdit->append("<span style=\" font-size:8pt; font-weight:600; color:#"+xmlData.values("color").front()+";\" >" + xmlData.values("text").front() + "</span>");
}

void MainWindow::sendXmlData(const std::list<std::pair<QString, QByteArray> > &data,std::list<QByteArray> namesToSend)
{

  QString message;

  std::vector<QString> receivers;
  receivers.reserve(namesToSend.size());

  for(const QString name : namesToSend ){

    receivers.push_back(name);
  }


  for(const auto&[attributeName, attributeValue] : data ){

    Packet packet;

    if(attributeName == "image"){

      std::vector<char> formatImg = {'b','m','p'};

      packet.setTypePacket(TypePacket::IMAGE);
      packet.setTypeDataAccess(TypeDataAccess::PRIVATE_DATA);
      packet.setData(attributeValue);
      packet.setMetaData(formatImg);
      packet.setReceivers(receivers);

      if(packet.isValid()){

        m_server.sendData(0,packet);

      }

    }else if(attributeName == "from"){
      message += attributeName + ':' + " ";
      message += attributeValue;
      message += '\n';

    }else if(attributeName == "color"){

      message += "<span style=\" font-size:8pt; font-weight:600; color:#"+attributeValue+";\" >";
    }
    else if(attributeName == "text"){

      message += attributeValue;
      message += "</span>";
    }
  }

  if(!message.isEmpty()){

    Packet packet;
    packet.setTypePacket(TypePacket::MESSAGE);
    packet.setTypeDataAccess(TypeDataAccess::PRIVATE_DATA);
    packet.setData(message.toUtf8());
    packet.setReceivers(receivers);

    if(packet.isValid()){
        m_server.sendData(0,packet);
    }
  }
}

void MainWindow::setEnableButtons(bool isEnable)
{
  ui->pushButtonLoadXml->setEnabled(isEnable);
  ui->pushButtonDisconnectClient->setEnabled(isEnable);
  ui->pushButtonStop->setEnabled(isEnable);
}

void MainWindow::on_pushButtonStop_clicked()
{
  m_server.close();

  setEnableButtons(false);
}



