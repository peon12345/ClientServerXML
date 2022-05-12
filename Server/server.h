#ifndef SERVER_H
#define SERVER_H


#include <winsock2.h>
#include <vector>
#include <atomic>
#include <QDebug>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../DataStruct/datastruct.h"
#include "../DataStruct/datahandler.h"

class Server : public QObject , public DataHandler{
  Q_OBJECT
public:
  Server() = default;
  virtual ~Server();

  Server(const Server&) = delete;
  Server(Server&&) = delete;
  Server& operator = (const Server&) = delete;
  Server& operator = (Server&&) = delete;

  void initServer(const std::string& ip,ushort port,const uint maxConnect = DEFAULT_MAX_CONNECT);
  void close();

  void startListen(bool listenNewConnect = true, bool listenClientSocket = true);
  void stopListenConnects();
  void stopListenClientSocket();

  void sendPacket(SOCKET socket,const Packet& packet);
  void disconnectClientByName(const QString& name);
protected:
  std::string m_ip;
  ushort m_port;

  void disconnectClientSocket(SOCKET socket);
private:
  static constexpr int DEFAULT_MAX_CONNECT = 100;
   size_t m_maxConnection;

  std::atomic<bool> m_flagListenConnects = false;
  std::atomic<bool> m_flagListenClientData = false;
  std::atomic<bool> m_continueListen = false;
  std::atomic<bool> m_needWaitData = false;
  std::mutex m_mutexStartListen;
  std::condition_variable m_cvListen;


  int m_counterUnknownClients = 0;
  SOCKET m_listenSocket;

  std::unordered_map<SOCKET,std::queue<Packet>> m_queueSendData;
  std::vector<std::pair<SOCKET,ClientInfo*>> m_connectedClients;
  std::vector<SOCKET> waitingDisconnectSocket;

private:
  void packetHandler(SOCKET socket,const Packet& packet) override;
  void addClientInfo(SOCKET socket,const Packet& packet);
  void newClientConnectHandler(SOCKET newConnection);
  void disconnectAll();
signals:
  void clientConnected(const QString& name);
  void packageReceived(const Packet& packet);
  void clientChangedName(const QString& oldname, const QString& newName);
};

#endif // SERVER_H
