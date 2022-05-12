#ifndef CLIENT_H
#define CLIENT_H
#include <winsock2.h>
#include <thread>
#include <ws2tcpip.h>
#include <QObject>
#include "../DataStruct/datastruct.h"
#include "../DataStruct/datahandler.h"


class Client : public QObject , public DataHandler
{
  Q_OBJECT
public:
  Client() = default;
  virtual ~Client();

  void connectToServer(const std::string &ip, const std::string &port);
  void disconnect();
  void listenServer();
  void sendToServer(const Packet& packet);

  void setName(const QString& name);
private:
  enum class ClientStatus {
    DISCONNECTED,
    CONNECTED
  }
  m_status;

  SOCKET m_connection;
private:
  ClientInfo m_clientInfo;
  void packetHandler(SOCKET socket,const Packet& packet) override;
private:
  void sendClientInfo();
  void emitPackageReceived(const Packet& packet);
signals:

void packageReceived(const Packet& packet);
};

#endif // CLIENT_H
