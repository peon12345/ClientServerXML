#include "client.h"
#include <QDebug>
#include "../DataStruct/datastruct.cpp"
#include "../DataStruct/datahandler.cpp"


Client::Client() : m_status(ClientStatus::DISCONNECTED)
{

}


Client::~Client()
{
  disconnect();
}

void Client::connectToServer(const std::string& ip,const std::string& port)
{
  WSAData wsData;
  WORD DllVersion = MAKEWORD(2,2);
  if(WSAStartup(DllVersion,&wsData) != 0 ){
    throw("ERROR_INIT");
  }

  ADDRINFO addr;
  addr.ai_family = AF_INET;
  addr.ai_socktype = SOCK_STREAM;
  addr.ai_protocol = IPPROTO_TCP;
  addr.ai_flags = AI_PASSIVE;

  ADDRINFO* m_resultAddr = nullptr;

  ZeroMemory(&addr,sizeof(addr));

  if(getaddrinfo(ip.c_str(),port.c_str(),&addr,&m_resultAddr) != 0 ){
    WSACleanup();
    qDebug() << GetLastError();
    throw("GETADDDRINFO_FAILED_WITH_ERROR");
  }
  m_connection = socket(m_resultAddr->ai_family,m_resultAddr->ai_socktype,m_resultAddr->ai_protocol);

  if(m_connection == INVALID_SOCKET){
    freeaddrinfo(m_resultAddr);
    WSACleanup();
    throw("INIT_CONNECTION_SOCKET_ERROR");
  }

  if(WSAAPI::connect(m_connection,m_resultAddr->ai_addr,static_cast<int>(m_resultAddr->ai_addrlen)) != 0)
  {
    WSACleanup();
    closesocket(m_connection);
    m_connection = INVALID_SOCKET;
    throw("CONNECT_ERROR");
  }

  m_status = ClientStatus::CONNECTED;

  sendClientInfo();
}

void Client::disconnect()
{
  WSACleanup();
  shutdown(m_connection,SD_BOTH);
  closesocket(m_connection);
  m_connection = INVALID_SOCKET;
  m_status = ClientStatus::DISCONNECTED;
}

void Client::listenServer()
{
  std::thread t( [&] () {


    while(m_status == ClientStatus::CONNECTED){ // цикл пока коннект

      try{

        recvPacket(m_connection);

      } catch(const QString& str){
        qDebug() << str;
        disconnect();
      }
    }

  });

  t.detach();
}

void Client::sendToServer(const Packet &packet)
{
  sendData(m_connection,packet);
}

void Client::packetHandler(SOCKET socket, const Packet &packet)
{
  Q_UNUSED(socket);

  switch (packet.type()) {
  case TypePacket::MESSAGE:{



    break;
  }

  case TypePacket::IMAGE:{

    break;
  }

  default:
    break;

  }
}

void Client::sendClientInfo()
{
  Packet packet;
  packet.setTypePacket(TypePacket::INFO_CLIENT);

  std::vector<char> data(sizeof(ClientInfo));

  memcpy(data.data(),&m_clientInfo,sizeof(ClientInfo));

  packet.setData(std::move(data));

  sendToServer(packet);
}
























