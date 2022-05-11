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

private:
  void xmlHandler(const QDomDocument& xmlDoc);
  void fillForm(const XmlData& xmlData);
  void sendDataToServer(const std::list<std::pair<QString,QByteArray>>& data);
private:
  void setEnableButtons(bool isEnable);
  Server m_server;
  QStringListModel* m_modelNewConnections;
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
