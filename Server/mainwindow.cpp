#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDomDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

}

MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::on_pushButtonStart_clicked()
{

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

  //QDomNodeList l_divs( xmlDoc.elementsByTagName( "FormatVersion" ) );


  QDomNodeList panelList = xmlDoc.elementsByTagName("Message");
  QDomNodeList panelCfgList = xmlDoc.elementsByTagName("FormatVersion");

  for ( int i = 0 ; i < panelList.size() ; ++i ) {
    QDomElement l_div( panelList.at( i ).toElement() );
    QString typeXml( l_div.attribute( "FormatVersion" ) );
    if(!typeXml.isEmpty()){
      format = static_cast<XmlParser::XmlFormatSupport>(typeXml.toInt());
      break;
    }

  }

  switch (format) {

  case XmlParser::XmlFormatSupport::MESSAGE_WITH_IMAGE:{

    std::list<std::pair<QString,std::vector<char>>> data = XmlParser::parse(xmlDoc);

    static const std::vector<QString> sendDataAttributes{"from","text","color","image"};


    break;
  }



    default:

    ui->textEdit->setText("this XML is not supported!");
    break;
  }
}





























