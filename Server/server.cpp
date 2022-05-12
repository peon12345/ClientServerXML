#include "server.h"
#include "../DataStruct/datastruct.cpp"
#include "../DataStruct/datahandler.cpp"

Server::~Server()
{
  closesocket(m_listenSocket);

  for( const auto &[socket , info] : m_connectedClients ){

    delete info;
    disconnectClientSocket(socket);
  }
  WSACleanup();
}

void Server::initServer(const std::string &ip, ushort port, const uint maxConnect)
{
  WSAData wsData;
  WORD DllVersion = MAKEWORD(2,2);
  if(WSAStartup(DllVersion,&wsData) !=0 ){
    throw("INIT_ERROR");
  }

  SOCKADDR_IN sockAddr;
  sockAddr.sin_addr.s_addr = inet_addr(ip.c_str());
  sockAddr.sin_port = htons(port);
  sockAddr.sin_family = AF_INET;

  m_listenSocket = socket(AF_INET,SOCK_STREAM,0);

  if(m_listenSocket == INVALID_SOCKET){
    WSACleanup();
    throw("LISTEN_SOCKET_INIT_ERROR");
  }

  if(bind(m_listenSocket,(SOCKADDR*)&sockAddr,sizeof(sockAddr))){
    closesocket(m_listenSocket);
    WSACleanup();
    throw("BIND_ERROR");
  }

  listen(m_listenSocket,maxConnect);

  if (listen(m_listenSocket,maxConnect) == SOCKET_ERROR) {
    closesocket(m_listenSocket);
    WSACleanup();
    throw("LISTEN_ERROR");
  }

  ulong NonBlockFlag = 1;
  if (ioctlsocket(m_listenSocket, FIONBIO, &NonBlockFlag) == SOCKET_ERROR)

  {
    closesocket(m_listenSocket);
    WSACleanup();
    throw("SET_UNBLOCK_LISTEN_SOCKET_ERROR");
  }

  m_ip = ip;
  m_port = port;
  m_maxConnection = maxConnect;
}

void Server::close()
{
  m_flagListenClientData = false;
  m_flagListenClientData = false;
  closesocket(m_listenSocket);
  WSACleanup();
}

void Server::startListen(bool listenNewConnect,bool listenClientSocket)
{

  m_flagListenConnects = listenNewConnect;
  m_flagListenClientData = listenClientSocket;
  static ulong NonBlockFlag = 1;

  std::thread t( [&]() {

    SOCKET clientSocket = INVALID_SOCKET;
    fd_set fdSetReady;
    fd_set fdSetWrite;
    fd_set fdSetError;

    while(m_flagListenConnects || m_flagListenClientData){
      FD_ZERO(&fdSetReady);
      FD_ZERO(&fdSetWrite);
      FD_ZERO(&fdSetError);

      if(m_flagListenConnects){
        FD_SET(m_listenSocket,&fdSetReady);
      }

      bool checkSendAllSocket = false;

      if(!m_queueSendData.empty()){

        auto it = m_queueSendData.find(0);

        if(it != m_queueSendData.end()){ // где нужно отправить всем
          checkSendAllSocket = true;

        }else{ //отправка конкретным клиентам

          for(std::pair<SOCKET,const std::queue<Packet>&> pair : m_queueSendData){
            FD_SET(pair.first,&fdSetWrite);
          }
        }
      }

      for(const auto& [socket , addr] : m_connectedClients){
        FD_SET(socket,&fdSetReady);
        FD_SET(socket,&fdSetError);

        if(checkSendAllSocket){
          FD_SET(socket,&fdSetWrite);
        }
      }

      int totalSocket = select(0,&fdSetReady,&fdSetWrite,&fdSetError,NULL);

      if(totalSocket == 0){
        continue;
      }
      else if(totalSocket == SOCKET_ERROR){
        return void();
      }


      if(m_flagListenConnects){
        if (FD_ISSET(m_listenSocket,&fdSetReady)){ // для нового клиента

          if(m_maxConnection > m_connectedClients.size()){
            if(clientSocket != 0){
              clientSocket = accept(m_listenSocket,NULL,NULL);
              if(clientSocket != INVALID_SOCKET)
              {
                if(ioctlsocket(clientSocket, FIONBIO, &NonBlockFlag) != SOCKET_ERROR)
                {
                  m_connectedClients.emplace_back(clientSocket,nullptr);

                  newClientConnectHandler(clientSocket);

                }else{
                  disconnectClientSocket(clientSocket);
                }
              }
            }
          }else{
            qDebug() << "new client is blocked";
          }
        }else{
          qDebug() << "CONNECT DONT FIND";
        }
      }

      if(m_flagListenClientData){

        for(size_t i = 0; i < m_connectedClients.size();++i ){

          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetError )){ // если ошибка
            //установим мьютекс
            //нужно очистить данные из очереди
            //в это время могли придти новые данные
            disconnectClientSocket(m_connectedClients.at(i).first);
            continue;
          }


          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetReady)){ //если есть данные для чтения

            try{
              recvPacket(m_connectedClients.at(i).first);
            }catch(const QString& str){

              disconnectClientSocket(m_connectedClients.at(i).first);
            }
          }

             //здесь установим мьютекс
             //в методе sendData можем добавить

          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetWrite)){ //если готова принять данные, проверяем на их наличие

            auto itCommonData = m_queueSendData.find(0);

            if(itCommonData != m_queueSendData.end()){

              //если данные приватны, проверяем, находится ли клиент в числе получателей
              if(itCommonData->second.front().typeAccess() == TypeDataAccess::PRIVATE_DATA){

                auto it = std::find_if(itCommonData->second.front().getReceivers().begin(),itCommonData->second.front().getReceivers().end(),
                                       [&connectedClients = m_connectedClients,i](const QString& name){

                                         return name == QString::fromLocal8Bit(connectedClients.at(i).second->getName().c_str());

                });

                if(it != itCommonData->second.front().getReceivers().end()){
                  sendData(m_connectedClients.at(i).first,itCommonData->second.front());
                }

              }else{
                sendData(m_connectedClients.at(i).first,itCommonData->second.front());

              }





            }

            auto it = m_queueSendData.find(m_connectedClients.at(i).first);
            if(it != m_queueSendData.end()){

              while(!it->second.empty()){

                sendData(m_connectedClients.at(i).first,it->second.front());
                it->second.pop();
              }
              if(it->first != 0){
                m_queueSendData.erase(it);
              }
            }
          }

             //если клиент не готов принять данные
             //но ему нужно их отправить, оставляем данные на потом, пока не получим ошибку клиентского сокета
             //узнаем кому не получилось отправить данные

          auto it = m_queueSendData.find(0);
          if(it != m_queueSendData.end()){

            for(size_t i = 0; i < m_connectedClients.size(); ++i){

              if(!FD_ISSET(m_connectedClients.at(i).first,&fdSetWrite)){

                   //данные, которые должны были быть отправлены всем
                   //делим на конкретных клиентов, которым не удалось отправить

                sendData(m_connectedClients.at(i).first,it->second.front());
              }

            }
          }

          m_queueSendData.erase(0); // удаляем общие данные
          //в m_queueSendData могут остаться данные, которые были предназначены конкретным клиентам
          //но пока не были отправлены
        }
      }

    }

  } );

  t.detach();

}

void Server::stopListenConnects()
{
  m_flagListenConnects = false;
}

void Server::stopListenClientSocket()
{
  m_flagListenClientData = false;
}





//если клиент инфо, кастить в клиент инфо
void Server::addClientInfo(SOCKET socket, const Packet &packet)
{


  ClientInfo* clientInfo = new ClientInfo();

  std::vector<char> temp = packet.getData();
  memcpy(*&clientInfo, temp.data(), sizeof(ClientInfo));

     //сохраним информацию клиента, привяжем к сокету
  auto it = std::find_if(m_connectedClients.begin() , m_connectedClients.end() ,[socket] (const std::pair<SOCKET,ClientInfo*>& pair)  {
    return  pair.first ==  socket;  });


  if(it != m_connectedClients.end()){

    if(it->second){
      delete it->second;
    }else{
     m_counterUnknownClients--;
     }

    it->second = clientInfo;

  }else{
    delete clientInfo;
  }
}







void Server::packetHandler(SOCKET socket, const Packet &packet)
{
  switch (packet.type()) {


  case TypePacket::INFO_CLIENT:{
    addClientInfo(socket,packet);
    break;
  }

  //...another Packet

    default:

    break;

  }


  emit packageReceived(packet);
}


void Server::sendData(SOCKET socket, const Packet &packet)
{

  auto it = m_queueSendData.find(socket);

  if(it != m_queueSendData.end()) {
    it->second.emplace(packet);
  }else{
    auto it = m_queueSendData.insert(std::make_pair(socket,std::queue<Packet>{}));
    it.first->second.emplace(packet);
    }


}

void Server::disconnectClientByName(const QString &name)
{
  std::string nameStd = name.toStdString();

  auto it = std::find_if(m_connectedClients.begin(),m_connectedClients.end(),
                         [&nameStd](const std::pair<SOCKET,ClientInfo*>& pair) {

    return (nameStd == pair.second->getName());

    });

  if(it != m_connectedClients.end()){
    disconnectClientSocket(it->first);
    }
}


void Server::disconnectClientSocket(SOCKET socket)
{
  //надо здесь мьютекс , в листен будет ошибка
  //здесь дожидаемся конца цикла в другом потоке

  auto it = std::find_if(m_connectedClients.begin(),
                         m_connectedClients.end(),
                         [socket](const std::pair<SOCKET,ClientInfo*>& pair) {return pair.first == socket;} );

  if(it != m_connectedClients.end()){

    delete it->second;
    m_connectedClients.erase(it);
    m_queueSendData.erase(socket);
  }

  shutdown(socket,SD_BOTH);
  closesocket(socket);
}

void Server::newClientConnectHandler(SOCKET newConnection)
{
  static const std::string connectedStr = "Server: Connection completed!";
  static const std::vector<char> message(connectedStr.begin(),connectedStr.end());

  static Packet packet;

  if(!packet.isValid()){
  packet.setTypePacket(TypePacket::MESSAGE);
  packet.setData(message);
  }

  sendData(newConnection,packet);
  //клиент еще не назвал свое имя, назовем его

  auto it = std::find_if(m_connectedClients.begin(),m_connectedClients.end(),[newConnection] (const std::pair<SOCKET,ClientInfo*>& pair) {

    return newConnection == pair.first;

    } );


  if(it != m_connectedClients.end()){
      QString name = "Anonim";
      if(m_counterUnknownClients > 0){
       name += "(" + QString::number(m_counterUnknownClients) + ")";
      }

     ClientInfo* clientInfo = new ClientInfo(name.toStdString());
     it->second = clientInfo;

     m_counterUnknownClients++;
     emit clientConnected(name);
  }
}



























