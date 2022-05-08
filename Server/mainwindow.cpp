#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDomDocument>
#include "xmlparserformat1.h"
#include "xmldata.h"
#include <QPixmap>

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


void MainWindow::on_pushButtonStart_clicked()
{

//  if(){
//    setEnableButtons(true);
//  }

}


void MainWindow::on_pushButtonDisconnectClient_clicked()
{
  QModelIndex index = ui->listViewConnectedClients->currentIndex();
  //QString itemText = index.data(Qt::DisplayRole).toString();
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

void MainWindow::xmlHandler(const QDomDocument &xmlDoc)
{
  //в будущем здесь можно узнать тип документа
  XmlParser::XmlFormatSupport format = XmlParser::XmlFormatSupport::MESSAGE_WITH_IMAGE;

  std::unique_ptr<XmlParser> xmlParser;
  std::vector<QString> sendAttributes;

  switch (format) {

  case XmlParser::XmlFormatSupport::MESSAGE_WITH_IMAGE:{

    xmlParser = std::make_unique<XmlParserFormat1>();

    static const std::vector<QString> sendNameAttributes{"from","text","color","image"};
    sendAttributes = sendNameAttributes;

    break;
  }


  //...
  //другие XML форматы

  default:

    ui->textEdit->setText("this XML is not supported!");
    return void();
  }

  //для формы заполним все что есть
  //для отправки настроим


  XmlData xmlData;

  try{
    xmlData = xmlParser->parse(xmlDoc);
  }catch(...){
    return void();
  }

  fillForm(xmlData);

}

void MainWindow::fillForm(const XmlData &xmlData)
{
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

void MainWindow::setEnableButtons(bool isEnable)
{
  ui->pushButtonLoadXml->setEnabled(isEnable);
  ui->pushButtonDisconnectClient->setEnabled(isEnable);
  ui->pushButtonStop->setEnabled(isEnable);
}





























