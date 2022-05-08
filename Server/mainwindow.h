#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "xmlreader.h"
#include "server.h"
#include "xmlparser.h"


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

private:
  void xmlHandler(const QDomDocument& xmlDoc);
  void fillForm(const XmlData& xmlData);
private:
  void setEnableButtons(bool isEnable);
 // Server m_server;
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
