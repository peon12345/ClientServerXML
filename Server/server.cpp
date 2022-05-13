#include "server.h"
#include "../DataStruct/datastruct.cpp"
#include "../DataStruct/datahandler.cpp"
#include <thread>
Server::~Server()
{
  close();
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
  m_flagListenConnects = false;
  //closesocket(m_listenSocket);
  disconnectAll();
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
    TIMEVAL tv{};
    tv.tv_sec = 5;

    while(m_flagListenConnects || m_flagListenClientData){


      for(SOCKET socket : m_waitingDisconnectSocket){
        disconnectClientSocket(socket);
      }
      m_waitingDisconnectSocket.clear();


      std::unique_lock ulk(m_mutexStartListen);
      if(m_needWaitData){
        m_cvListen.wait(ulk,[&b = m_continueListen]{return b == true;});
        m_needWaitData = false;
      }

      FD_ZERO(&fdSetReady);
      FD_ZERO(&fdSetWrite);
      FD_ZERO(&fdSetError);


      if(m_flagListenConnects){
        FD_SET(m_listenSocket,&fdSetReady);
      }

      bool checkSendAllSocket = false;

      if(!m_sendData.empty()){

        auto it = m_sendData.find(0);

        if(it != m_sendData.end()){ // где нужно отправить всем
          checkSendAllSocket = true;

        }else{ //отправка конкретным клиентам

          for(std::pair<SOCKET,const std::list<Packet>&> pair : m_sendData){
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

      int totalSocket = select(0,&fdSetReady,&fdSetWrite,&fdSetError,&tv);

      if((totalSocket == 0 || totalSocket == SOCKET_ERROR) || (fdSetReady.fd_count == 0 && m_sendData.empty())){
        continue;
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

      if(m_connectedClients.empty()){
        continue;
      }

      if(m_flagListenClientData){

        for(size_t i = 0; i < m_connectedClients.size();++i ){

          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetError )){ // если ошибка

            scheduleShutdownSocket(m_connectedClients.at(i).first);
            continue;
          }


          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetReady)){ //если есть данные для чтения

            try{
              recvPacket(m_connectedClients.at(i).first);
            }catch(const char* str){

              scheduleShutdownSocket(m_connectedClients.at(i).first);
              continue;
            }
          }


          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetWrite)){ //если готова принять данные, проверяем на их наличие

            auto itCommonData = m_sendData.find(0);

            if(itCommonData != m_sendData.end()){

                 //если данные приватны, проверяем, находится ли клиент в числе получателей
              if(itCommonData->second.front().typeAccess() == TypeDataAccess::PRIVATE_DATA){

                auto it = std::find_if(itCommonData->second.front().getReceivers().begin(),itCommonData->second.front().getReceivers().end(),
                                       [&connectedClients = m_connectedClients,i](const QString& name){
                                         qDebug() << "compare:" << name.toLocal8Bit();
                                         qDebug() << QString::fromStdString(connectedClients.at(i).second->getName());
                                         return name == QString::fromStdString(connectedClients.at(i).second->getName());

                                       });

                if(it != itCommonData->second.front().getReceivers().end()){

                  for(const Packet& packet: itCommonData->second){
                    sendData(m_connectedClients.at(i).first,packet);
                  }


                }

              }else{

                for(const Packet& packet: itCommonData->second){
                  sendData(m_connectedClients.at(i).first,packet);
                }

              }

            }

            auto it = m_sendData.find(m_connectedClients.at(i).first);
            if(it != m_sendData.end()){

              for(const Packet& packet : it->second){

                sendData(m_connectedClients.at(i).first,packet);
              }


              if(it->first != 0){
                m_sendData.erase(it);
              }
            }
          }

             //если клиент не готов принять данные
             //но ему нужно их отправить, оставляем данные на потом, пока не получим ошибку клиентского сокета
             //узнаем кому не получилось отправить данные

          auto it = m_sendData.find(0);
          if(it != m_sendData.end()){

            for(size_t i = 0; i < m_connectedClients.size(); ++i){

              if(!FD_ISSET(m_connectedClients.at(i).first,&fdSetWrite)){

                   //данные, которые должны были быть отправлены всем
                   //делим на конкретных клиентов, которым не удалось отправить

                sendPacket(m_connectedClients.at(i).first,it->second.front());
              }

            }
          }

          m_sendData.erase(0); // удаляем общие данные
          //в m_sendData могут остаться данные, которые были предназначены конкретным клиентам
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

  memcpy(*&clientInfo, packet.getData().data(), sizeof(ClientInfo));

     //сохраним информацию клиента, привяжем к сокету
  auto it = std::find_if(m_connectedClients.begin() , m_connectedClients.end() ,[socket] (const std::pair<SOCKET,ClientInfo*>& pair)  {
    return  pair.first ==  socket;  });


  QString oldName;
  if(it != m_connectedClients.end()){

    oldName = QString::fromLocal8Bit(it->second->getName().c_str());

    if(it->second){
      delete it->second;
    }else{
      m_counterUnknownClients--;
    }

    it->second = clientInfo;

    emit clientChangedName(oldName,QString::fromLocal8Bit(clientInfo->getName().c_str()).toUtf8());

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
  case TypePacket::COMMAND:{

    if(!packet.getMetaData().empty()){
      int8_t command = packet.getMetaData().back();

      switch (static_cast<Command>(command)) {

      case Command::SEND_AGAIN_XML_DATA:{
        emit requestXmlData();
      }

      }
    }
  }

       //...another Packet

  default:

    break;

  }

  emit packageReceived(packet);
}


void Server::sendPacket(SOCKET socket, const Packet &packet)
{
  m_needWaitData = true;
  m_continueListen = false;

  auto it = m_sendData.find(socket);

  if(it != m_sendData.end()) {
    it->second.push_back(packet);
  }else{
    auto it = m_sendData.insert(std::make_pair(socket,std::list<Packet>{}));
    it.first->second.push_back(packet);
  }

  m_continueListen = true;
  m_cvListen.notify_one();
}

void Server::disconnectClientByName(const QString &name)
{
  std::string nameStd = name.toStdString();

  auto it = std::find_if(m_connectedClients.begin(),m_connectedClients.end(),
                         [&nameStd](const std::pair<SOCKET,ClientInfo*>& pair) {

                           return (nameStd == pair.second->getName());

                         });

  if(it != m_connectedClients.end()){
    m_waitingDisconnectSocket.push_back(it->first);
  }
}


void Server::disconnectClientSocket(SOCKET socket)
{
  if(shutdown(socket,SD_BOTH ) != SOCKET_ERROR){

    closesocket(socket);
  }

  auto it = std::find_if(m_connectedClients.begin(),
                         m_connectedClients.end(),
                         [socket](const std::pair<SOCKET,ClientInfo*>& pair) {return pair.first == socket;} );

  if(it != m_connectedClients.end()){
    QString name = QString::fromLocal8Bit(it->second->getName().c_str());
    delete it->second;
    m_connectedClients.erase(it);
    m_sendData.erase(socket);

    emit clientDisconnected(name);
  }

}

void Server::newClientConnectHandler(SOCKET newConnection)
{
  static const std::string connectedStr = "Server: Connection completed!";
  static const std::vector<char> message(connectedStr.begin(),connectedStr.end());


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


    static Packet packet;

    if(!packet.isValid()){
      packet.setTypePacket(TypePacket::MESSAGE);
      packet.setTypeDataAccess(TypeDataAccess::PRIVATE_DATA);
      packet.appendReceiver(QString::fromLocal8Bit(it->second->getName().c_str()));
      packet.setData(message);
    }

    sendPacket(newConnection,packet);
    m_counterUnknownClients++;
    emit clientConnected(name);
  }
}

void Server::disconnectAll()
{
  for( const auto &[socket , info] : m_connectedClients ){
    delete info;
    scheduleShutdownSocket(socket);
    //disconnectClientSocket(socket);
  }
}

void Server::scheduleShutdownSocket(SOCKET socket)
{
  m_waitingDisconnectSocket.push_back(socket);
}



























