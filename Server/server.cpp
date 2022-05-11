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
            disconnectClientHandler(m_connectedClients.at(i));
            disconnectClientSocket(m_connectedClients.at(i).first);
            continue;
          }


          if(FD_ISSET(m_connectedClients.at(i).first,&fdSetReady)){ //если есть данные для чтения

            try{
              recvPacket(m_connectedClients.at(i).first);
              recvDataClientHanlder(m_data,m_header,m_connectedClients.at(i).second);
            }catch(const QString& str){

              disconnectClientSocket(m_connectedClients.at(i).first);
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


void Server::disconnectClientHandler(std::pair<SOCKET, const ClientInfo*> disconnectClient)
{
  static const std::string disconnectedStr = "Server: disconnected you!";

  Packet packet;
  packet.setTypePacket(TypePacket::MESSAGE);
  packet.setTypeDataAcces(TypeDataAccess::PRIVATE_DATA);
  packet.appendReceiver(QString::fromLocal8Bit(disconnectClient.second->getName().c_str()));

  std::vector<char> temp(disconnectedStr.begin(),disconnectedStr.end());
  packet.setData(std::move(temp));

  sendData(disconnectClient.first,packet);
}

void Server::sendDataClientHandler(const Packet &data, std::pair<SOCKET, const ClientInfo*> sendDataClientInfo)
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


void Server::recvPacket(SOCKET socket)
{
  Packet packet;

  char type;
  int resultRecv = recv(socket,&type,PacketHeader::LEN_TYPE_PACKET,0);

  if(resultRecv < 0){

    throw "TYPE_READ_ERROR";
  }else{

    packet.setTypePacket(static_cast<TypePacket>(type));

    char typeAccess = static_cast<char>(TypeDataAccess::PUBLIC_DATA);

    if(TypePacket::INFO_CLIENT != packet.type()){

      resultRecv = recv(socket,&typeAccess,PacketHeader::LEN_TYPE_DATA_ACCESS,0);

      if(resultRecv < 0){
         throw "TYPE_ACCESS_READ_ERROR";
      }else{
        packet.setTypeDataAcces(static_cast<TypeDataAccess>(typeAccess));
      }
    }

    if(packet.typeAccess() != TypeDataAccess::PUBLIC_DATA){
      char sizeReceivers = 0;

      resultRecv = recv(socket,&sizeReceivers,PacketHeader::LEN_COUNT_NAME,0);

      if(resultRecv < 0){
         throw "SIZE_RECEIVERS_READ_ERROR";
      }else{

        std::vector<char> name(ClientInfo::MAX_LENGHT_NAME);
        for(int i = 0; i < static_cast<uint8_t>(sizeReceivers); ++i){
          name.clear();
          if(recvData(socket,name,ClientInfo::MAX_LENGHT_NAME)){

            packet.appendReceiver(QString::fromUtf8(name.data()));

          }else{
             throw "ERROR_READ_RECEIVERS_NAME";
          }
        }
      }
    }



    switch (packet.type()) {


    case TypePacket::IMAGE:
    case TypePacket::MESSAGE:{

      if(TypePacket::IMAGE == packet.type()){
        std::vector<char> formatImg(Packet::LEN_FORMAT_IMAGE);
        if(recvData(socket,formatImg,Packet::LEN_FORMAT_IMAGE)){

          packet.setMetaData(formatImg);
        }else{

          throw "READ_FORMAT_IMAGE_ERROR";
        }
      }

      std::vector<char> sizeData(Packet::LEN_SIZE_DATA_INFO);

      if(recvData(socket,sizeData,Packet::LEN_SIZE_DATA_INFO)){
        packet.setSize(sizeData);
      }else{

        throw "SIZE_DATA_READ_ERROR";
      }

      size_t size =0;

      std::for_each(sizeData.begin(),sizeData.end(), [&size] (char s) {
        size+= (uint8_t)s;
      });

      std::vector<char> data(size);

      if(recvData(socket,data,size)){

        packet.setData(std::move(data));
      }else{

        throw "ERROR_DATA_READ";
      }

    }

    case TypePacket::INFO_CLIENT:{


      std::vector<char> clientInfoData(sizeof(ClientInfo));

      if(recvData(socket,clientInfoData,sizeof(ClientInfo))){

        packet.setData(std::move(clientInfoData));
      }
    }
    default:{

      throw "UNSUPPORTED PACKAGE";

      break;
    }

    }

    packetHandler(packet);

    emit packageReceived(packet);
  }
}



//если клиент инфо, кастить в клиент инфо
void Server::addClientInfo(const Packet &packet)
{
  if(!Data::recvData(packet,m_data,sizeof(ClientInfo))){

    disconnectClientSocket(packet);
  }

  ClientInfo* clientInfo = new ClientInfo();
  memcpy(*&clientInfo, m_data.data(), sizeof(ClientInfo));

     //сохраним информацию клиента, привяжем к сокету
  auto it = std::find_if(m_connectedClients.begin() , m_connectedClients.end() ,[packet] (const std::pair<SOCKET,ClientInfo*>& pair)  {
    return  pair.first ==  packet;  });


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

void Server::startSendData(SOCKET socket, const Packet &packet)
{
  size_t sendResult = 0;
  size_t sizeData = 0;
  do{
    sizeData =  packet.size();
    sendResult = send(socket,packet.data(),sizeData,0);
    }while(sendResult < sizeData || static_cast<int>(sendResult) != SOCKET_ERROR);
}

bool Server::recvData(SOCKET socket, std::vector<char> &dataOutput, size_t size)
{
    if(dataOutput.size() < size){
        dataOutput.resize(size);
      }

    int total = 0;
    int resultRecv = 0;
    int sizeLeft = size;

    while(total < sizeLeft) {
        resultRecv = recv(socket,dataOutput.data(),sizeLeft,0);

        if (resultRecv == 0) {
            break;
          }else if(resultRecv < 0){

            return false;
          }

        total += resultRecv;
        sizeLeft -= resultRecv;
      }
    return true;
}

void Server::packetHandler(const Packet &packet)
{
  switch (packet.type()) {


  case TypePacket::INFO_CLIENT:{
    addClientInfo(packet);
    break;
  }

  //...another Packet

    default:

    break;

  }
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



























