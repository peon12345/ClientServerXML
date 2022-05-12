#include "datahandler.h"




void DataHandler::recvData(SOCKET socket, std::vector<char> &dataOutput, size_t size)
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

      throw "ERROR_READ";
    }

    total += resultRecv;
    sizeLeft -= resultRecv;
  }

}


void DataHandler::sendData(SOCKET socket, const Packet &packet)
{
  size_t sendResult = 0;
  size_t sizeData = 0;

  std::vector<char> data = packet.convertToVector();

  do{
    sizeData =  data.size();
    sendResult = send(socket,data.data(),sizeData,0);
  }while(sendResult < sizeData || static_cast<int>(sendResult) != SOCKET_ERROR);
}



void DataHandler::recvPacket(SOCKET socket)
{
  Packet packet;

  char type;
  int resultRecv = recv(socket,&type,PacketHeader::LEN_TYPE_PACKET,0);

  if(resultRecv > 0){
    std::vector<char> sizePacket( Packet::sizePacketBuffLen(static_cast<TypePacket>(type)));

    try {
      recvData(socket,sizePacket,sizePacket.size());
     }catch(const QString& str){
      throw str;
     }

      size_t size = 0;

      std::for_each(sizePacket.begin(),sizePacket.end(), [&size](char s) {
        size += uint8_t(s);
      });
      std::vector<char> data(size);
      try{
        recvData(socket,data,size);
       }catch(const QString& str){

        throw str;
       }

        packet.setDataWithHeader(data);

      }

  packetHandler(socket,packet);

}
