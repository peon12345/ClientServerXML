#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "xmlreader.h"
#include "server.h"
#include "xmlparser.h"
#include <QStringListModel>

QT_BEGIN_NAMESPACE
    namespace Ui { class MainWindow; }
QT_END_NAMESPACE

 class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_pushButtonStart_clicked();

  void on_pushButtonDisconnectClient_clicked();

  void on_pushButtonLoadXml_clicked();
  void addNewClient(const QString& name);

  void on_pushButtonStop_clicked();
  void updateNameClient(const QString& oldName, const QString& newName);
  void eraseClient(const QString& name);

private:
  void xmlHandler(const QDomDocument& xmlDoc);
  void fillForm(const XmlData& xmlData);
  void sendXmlData(const std::list<std::pair<QString,QByteArray>>& data, std::list<QByteArray> namesToSend);
private:
  void setEnableButtons(bool isEnable);
  Server m_server;
  XmlData m_xmlData;
  QStringListModel* m_modelNewConnections;
  std::vector<QString> m_attributesToSendServer;
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
