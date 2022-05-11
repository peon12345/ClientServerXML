#ifndef CLIENT_H
#define CLIENT_H
#include <winsock2.h>
#include <thread>
#include <ws2tcpip.h>
#include <QObject>
#include "../DataStruct/datastruct.h"


class Client
{
public:
  Client();
  Client(const std::string& name);
  virtual ~Client();

  void connectToServer(const std::string &ip, const std::string &port);
  void disconnect();
  void listenServer();
  bool sendToServer(std::vector<char>& data , TypePacket type = TypePacket::UNKNOWN, bool fillHeader = false);
  bool sendToServer(const std::vector<char>& data);
private:
  enum class ClientStatus {
    DISCONNECTED,
    CONNECTED
  }
  m_status;

  SOCKET m_connection;

  std::vector<char> m_data;
  ClientInfo m_infoToServer;
protected:
  virtual void recvServerDataHandler(std::vector<char> &data , std::vector<char>& header);
};

#endif // CLIENT_H
