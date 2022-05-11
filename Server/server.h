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

  void sendData( std::vector<char>& data , SOCKET socket = 0 , TypePacket type = TypePacket::UNKNOWN , bool fillHeader = false);
  void sendData( SOCKET socket,const std::vector<char>& data );

  void disconnectClientByName(const QString& name);
protected:
  std::string m_ip;
  ushort m_port;
protected:
  void disconnectClientSocket(SOCKET socket);
  std::optional<SOCKET> findSocketClient(const std::string& nameClient );

  virtual void newClientConnectHandler(SOCKET newConnection);
  virtual void disconnectClientHandler(std::pair<SOCKET,const ClientInfo*> disconnectClient);
  virtual void sendDataClientHandler(const std::vector<char>& data,std::pair<SOCKET,const ClientInfo*> sendDataClientInfo );
  virtual void recvDataClientHanlder(const std::vector<char>& data,const std::vector<char>& header,const ClientInfo* client );
private:
  static constexpr int DEFAULT_MAX_CONNECT = 100;

  std::atomic<bool> m_flagListenConnects = false;
  std::atomic<bool> m_flagListenClientData = false;
  int m_counterUnknownClients = 0;
  SOCKET m_listenSocket;
  size_t m_maxConnection;

  std::unordered_map<SOCKET,std::queue<std::vector<char>>> m_queueSendData;
  std::vector<std::pair<SOCKET,ClientInfo*>> m_connectedClients;

  std::vector<char> m_data;
  std::vector<char> m_header;

private:
  void startRecvPacket(SOCKET socket);
  void recvInfoPacket(SOCKET socket);
  void recvDataPacket(SOCKET socket,bool isPrivate = false);
  void startSendData(SOCKET socket,std::vector<char>& data);
  bool startRecvData(SOCKET socket ,std::vector<char>& dataOutput , size_t size);

  std::optional<QString> findNameBySocket(SOCKET socket);
  std::optional<SOCKET> findSocketByName(const QString& name);
signals:
  void clientConnected(const QString& name);

};

#endif // SERVER_H
