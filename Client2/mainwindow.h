#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "client.h"
#include <QMainWindow>


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

  void on_pushButtonConnect_clicked();

  void on_pushButtonRefresh_clicked();
  void fillForm(const Packet& packet);
  void on_pushButtonDisconnect_clicked();

private:
  Client m_client;
  void setEnableButtons(bool isEnable);
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
