#include "server.h"

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

          for(std::pair<SOCKET,const std::queue<std::vector<char>>&> pair : m_queueSendData){
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
            disconnectClientHandler(m_connectedClients.at(i));
            disconnectClientSocket(m_connectedClients.at(i).first);
            continue;
          }


          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetReady)){ //если есть данные для чтения

            startRecvPacket(m_connectedClients.at(i).first);

            if(!m_connectedClients.empty()){
              recvDataClientHanlder(m_data,m_header,m_connectedClients.at(i).second);
            }
          }

             //здесь установим мьютекс
             //в методе sendData можем добавить

          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetWrite)){ //если готова принять данные, проверяем на их наличие

            auto itCommonData = m_queueSendData.find(0);

            if(itCommonData != m_queueSendData.end()){

              startSendData(m_connectedClients.at(i).first,itCommonData->second.front());
              sendDataClientHandler(itCommonData->second.front() , m_connectedClients.at(i));
            }

            auto it = m_queueSendData.find(m_connectedClients.at(i).first);
            if(it != m_queueSendData.end()){

              while(!it->second.empty()){

                startSendData(m_connectedClients.at(i).first,it->second.front());
                sendDataClientHandler(it->second.front() , m_connectedClients.at(i));
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

                sendData(it->second.front(),m_connectedClients.at(i).first);
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


void Server::disconnectClientHandler(std::pair<SOCKET, const ClientInfo*> disconnectClient)
{
  static const std::string disconnectedStr = "Server: disconnected you!";
  static std::vector<char> msgDisconnected(Data::sizeHeader(TypePacket::MESSAGE));

  if(!msgDisconnected.empty()) {
    Data::fillHeader(msgDisconnected,TypePacket::MESSAGE,disconnectedStr.length());
    msgDisconnected.insert(msgDisconnected.end(),disconnectedStr.begin(),disconnectedStr.end());
  }

  std::string noticeDisconnectionStr = disconnectClient.second->getName() + " - disconnected";

  std::vector<char> msgDisconnectedNotice;
  Data::fillHeader(msgDisconnectedNotice,TypePacket::MESSAGE, noticeDisconnectionStr.length());
  msgDisconnectedNotice.insert(msgDisconnectedNotice.end(),noticeDisconnectionStr.begin(),noticeDisconnectionStr.end());

  sendData(msgDisconnected,0,TypePacket::MESSAGE);
}

void Server::sendDataClientHandler(const std::vector<char> &data, std::pair<SOCKET, const ClientInfo*> sendDataClientInfo)
{

}

void Server::recvDataClientHanlder(const std::vector<char> &data,const std::vector<char>& header, const ClientInfo *client)
{

  if(!data.empty()){
    //данные приняты
    //отправим всем клиентам

    if(header.front() == static_cast<char>(TypePacket::MESSAGE)){

      std::vector<char> translatedData(Data::sizeHeader(TypePacket::MESSAGE));

      std::string name = client->getName() + ":";

      translatedData.insert(translatedData.end(),name.begin(),name.end());
      translatedData.insert(translatedData.end(),data.begin(),data.end());

      sendData(translatedData,0,TypePacket::MESSAGE,true);

    }
  }
}


void Server::startRecvPacket(SOCKET socket)
{
  m_header.clear();
  m_data.clear();

     // узнаем тип пакета
  char type;
  int resultRecv = recv(socket,&type,LEN_TYPE_PACKET,0);

  if(resultRecv < 0){
    disconnectClientSocket(socket);
    qDebug() <<  socket <<"disconect";

  }else{

    auto it = std::find_if(m_connectedClients.begin() , m_connectedClients.end() , [socket](const std::pair<SOCKET,ClientInfo*>& pair) {
      return pair.first == socket;} ) ;

    if(!it->second && type != static_cast<char>(TypePacket::INFO_CLIENT)){
      //если клиент не представился и шлет нам что то
    }

    m_header.push_back(type);

    switch (static_cast<TypePacket>(type)) {

    case TypePacket::INFO_CLIENT:{
      qDebug() << "RECV_INFO_CLIENT_TYPE";
      recvInfoPacket(socket);
      break;
    }

    case TypePacket::MESSAGE:{
      recvDataPacket(socket);
      break;
    }

    case TypePacket::PRIVATE_DATA:{
      recvDataPacket(socket,true);
    }

    default:
      break;
    }
  }
}


//если клиент инфо, кастить в клиент инфо
void Server::recvInfoPacket(SOCKET socket)
{
  if(!Data::recvData(socket,m_data,sizeof(ClientInfo))){

    disconnectClientSocket(socket);
  }

  ClientInfo* clientInfo = new ClientInfo();
  memcpy(*&clientInfo, m_data.data(), sizeof(ClientInfo));

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

void Server::recvDataPacket(SOCKET socket, bool isPrivate)
{
  char countName;

  if(isPrivate){
    if(recv(socket,&countName,LEN_COUNT_NAME,0) < 0){
      disconnectClientSocket(socket);
      return void();
    }else{
      m_header.push_back(countName);
    }
  }

  std::vector<char> sizeBuf(LEN_SIZE_DATA_INFO);

  if(!Data::recvData(socket,sizeBuf,LEN_SIZE_DATA_INFO)){
    disconnectClientSocket(socket);
    return void();
  }else{
    m_header.insert(m_header.end(),sizeBuf.begin(),sizeBuf.end());
  }

  std::vector<char> names;
  if(isPrivate){// получим список имен, кому надо отправить
    uint8_t sizeName = countName;
    sizeName *= ClientInfo::MAX_LENGHT_NAME;

    names.resize(sizeName);
    if(!Data::recvData(socket,names,sizeName)) {
      disconnectClientSocket(socket);
      return void();
    }else{
      m_header.insert(m_header.end(),names.begin(),names.end());
    }
  }

  size_t size = Data::accumulateSize(sizeBuf);

  if(!Data::recvData(socket,m_data,size)){

    disconnectClientSocket(socket);
  }
}

void Server::startSendData(SOCKET socket, std::vector<char> &data)
{
  size_t sendResult = 0;
  size_t sizeData = 0;
  do{
    sizeData =  data.size();
    sendResult = send(socket,data.data(),sizeData,0);
  }while(sendResult < sizeData || static_cast<int>(sendResult) != SOCKET_ERROR);
}

void Server::sendData(std::vector<char> &data, SOCKET socket, TypePacket type,bool fillHeader)
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

    if(!memReserve){ //не оставили память для заголовка
      std::vector<char> header;
      Data::fillHeader(header,type,data.size());
      data.insert(data.cbegin(),header.begin(),header.end());
    }else{
      Data::fillHeader(data,type,data.size() - Data::sizeHeader(type));
    }
  }

  sendData(socket,data);
}

void Server::sendData(SOCKET socket, const std::vector<char> &data)
{
  auto it = m_queueSendData.find(socket);

  if(it != m_queueSendData.end()) {
    it->second.emplace(data);
  }else{
    auto it = m_queueSendData.insert(std::make_pair(socket,std::queue<std::vector<char>>{}));
    it.first->second.emplace(data);
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

std::optional<SOCKET> Server::findSocketClient(const std::string &nameClient)
{
  auto it = std::find_if(m_connectedClients.begin(),
                         m_connectedClients.end(),
                         [&nameClient](const std::pair<SOCKET,ClientInfo*>& pair) {return pair.second->getName() == nameClient;} );

  if(it != m_connectedClients.end() ) {

    return it->first;
  }else{
    return std::nullopt;
  }
}

void Server::newClientConnectHandler(SOCKET newConnection)
{
  static const std::string connectedStr = "Server: Connection completed!";
  static std::vector<char> msgConnected(Data::sizeHeader(TypePacket::MESSAGE));
  msgConnected.insert(msgConnected.end(),connectedStr.begin(),connectedStr.end());
  sendData(msgConnected,newConnection,TypePacket::MESSAGE , true);

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



























