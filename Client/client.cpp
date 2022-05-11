#include "client.h"
#include <QDebug>
#include "../DataStruct/datastruct.cpp"


Client::Client() : m_status(ClientStatus::DISCONNECTED)
{

}

Client::Client(const std::string& name) : m_status(ClientStatus::DISCONNECTED),m_infoToServer(name)
{
}

Client::~Client()
{
  disconnect();
}

void Client::connectToServer(const std::string& ip,const std::string& port)
{
  WSAData wsData;
  WORD DllVersion = MAKEWORD(2,1);
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

  char infoClient[sizeof(ClientInfo)];
  memcpy(infoClient,&m_infoToServer,sizeof(ClientInfo));

  std::vector<char> dataClientInfo;
  Data::fillHeader(dataClientInfo, TypePacket::INFO_CLIENT);
  dataClientInfo.insert(dataClientInfo.end(),infoClient,infoClient+sizeof(ClientInfo));

  if( !sendToServer(dataClientInfo) ){

    qDebug() << "sendError";
  }
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

    char typePacket;

    std::vector<char> sizeBuf(LEN_SIZE_DATA_INFO);
    size_t sizeData = 0;

    while(m_status == ClientStatus::CONNECTED){ // цикл пока коннект

      typePacket = recv(m_connection,&typePacket,LEN_TYPE_PACKET,0);

      m_data.clear();

      switch (static_cast<TypePacket>(typePacket)) {
      case TypePacket::DATA: {

        std::fill(sizeBuf.begin(),sizeBuf.end(),0);

        if(Data::recvData(m_connection,sizeBuf,LEN_SIZE_DATA_INFO)){
          sizeData = Data::accumulateSize(sizeBuf);
        }else{
          continue;
        }

        if(Data::recvData(m_connection,m_data,sizeData)){
          //получили данные и что то можно с ними делать

          std::vector<char> header;
          header.push_back(typePacket);
          header.insert(header.end(),sizeBuf.begin(),sizeBuf.end());

          recvServerDataHandler(m_data,header);
        }else{
          disconnect();
        }
      }

      default:
        break;
      }
    }

  });

  t.detach();
}

bool Client::sendToServer(std::vector<char> &data, TypePacket type , bool fillHeader)
{

  if(type != TypePacket::UNKNOWN && fillHeader){

    uint8_t sizeHeader = Data::sizeHeader(type);
    bool memReserve = false;


    if( sizeHeader <= data.size()){
      for(uint8_t i = 0; i < sizeHeader; ++i){
        if(data.at(i) == '\0'){
          memReserve = true;
        }else{
          memReserve = false;
          break;
        }
      }
    }

    if(!memReserve){ //не оставили память для загаловка
      std::vector<char> header;
      Data::fillHeader(header,type,data.size());
      data.insert(data.cbegin(),header.begin(),header.end());
    }else{
      Data::fillHeader(data,type,data.size() - Data::sizeHeader(type));
    }
  }

  return (send(m_connection,data.data(),data.size(),0) != SOCKET_ERROR);
}

bool Client::sendToServer(const std::vector<char> &data)
{
  return (send(m_connection,data.data(),data.size(),0) != SOCKET_ERROR);
}

void Client::recvServerDataHandler(std::vector<char> &data, std::vector<char>& header )
{
  qDebug() << "client rec :"<< data;
}



















