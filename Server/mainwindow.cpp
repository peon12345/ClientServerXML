#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDomDocument>
#include "xmlparserformat1.h"
#include "xmldata.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->lineEditIP->setText("127.0.0.1");
  ui->lineEditPort->setText("8005");
  //setEnableButtons(false);
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

  XmlParser::XmlFormatSupport format = XmlParser::XmlFormatSupport::UNIDENTIFIED;

  QDomNodeList panelList = xmlDoc.elementsByTagName("Message");

  for ( int i = 0 ; i < panelList.size() ; ++i ) {
    QDomElement l_div( panelList.at( i ).toElement() );
    QString typeXml( l_div.attribute( "FormatVersion" ) );
    if(!typeXml.isEmpty()){
      format = static_cast<XmlParser::XmlFormatSupport>(typeXml.toInt());
      break;
    }

  }

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

  }

  //данные получены

}

void MainWindow::setEnableButtons(bool isEnable)
{
  ui->pushButtonLoadXml->setEnabled(isEnable);
  ui->pushButtonDisconnectClient->setEnabled(isEnable);
  ui->pushButtonStop->setEnabled(isEnable);
}





























