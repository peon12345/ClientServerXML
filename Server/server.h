#ifndef SERVER_H
#define SERVER_H


#include <winsock2.h>
#include <vector>
#include <atomic>
#include <QDebug>
#include <thread>
#include <queue>
#include <mutex>
#include "../DataStruct/datastruct.h"

class Server : public QObject {
  Q_OBJECT
public:
  Server() = default;
  virtual ~Server();

  Server(const Server&) = delete;
  Server(Server&&) = delete;
  Server& operator = (const Server&) = delete;
  Server& operator = (Server&&) = delete;

  void initServer(const std::string& ip,ushort port,const uint maxConnect = DEFAULT_MAX_CONNECT);

  void startListen(bool listenNewConnect = true, bool listenClientSocket = true);
  void stopListenConnects();
  void stopListenClientSocket();

  void sendData( SOCKET socket,const Packet& packet );

  void disconnectClientByName(const QString& name);
protected:
  std::string m_ip;
  ushort m_port;
protected:
  void disconnectClientSocket(SOCKET socket);
  std::optional<SOCKET> findSocketClient(const std::string& nameClient );

  virtual void newClientConnectHandler(SOCKET newConnection);
  virtual void disconnectClientHandler(std::pair<SOCKET,const ClientInfo*> disconnectClient);
  virtual void sendDataClientHandler(const Packet& data,std::pair<SOCKET,const ClientInfo*> sendDataClientInfo );
  virtual void recvDataClientHanlder(const std::vector<char>& data,const std::vector<char>& header,const ClientInfo* client );
private:
  static constexpr int DEFAULT_MAX_CONNECT = 100;

  std::atomic<bool> m_flagListenConnects = false;
  std::atomic<bool> m_flagListenClientData = false;
  int m_counterUnknownClients = 0;
  SOCKET m_listenSocket;
  size_t m_maxConnection;

  std::unordered_map<SOCKET,std::queue<Packet>> m_queueSendData;
  std::vector<std::pair<SOCKET,ClientInfo*>> m_connectedClients;
private:
  void recvPacket(SOCKET socket);
  bool recvData(SOCKET socket ,std::vector<char>& dataOutput , size_t size);

  void packetHandler(const Packet& packet);

  void addClientInfo(const Packet& packet);
  void recvDataPacket(SOCKET socket,bool isPrivate = false);
  void startSendData(SOCKET socket,const Packet& packet);


  std::optional<QString> findNameBySocket(SOCKET socket);
  std::optional<SOCKET> findSocketByName(const QString& name);
signals:
  void clientConnected(const QString& name);
  void packageReceived(const Packet& packet);

};

#endif // SERVER_H
