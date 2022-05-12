#ifndef DATAHANDLER_H
#define DATAHANDLER_H
#include "datastruct.h"


class DataHandler
{
public:
  DataHandler() = default;
  virtual ~DataHandler() = default;
protected:
  void recvData(SOCKET socket ,std::vector<char>& dataOutput , size_t size);
  void sendData(SOCKET socket,const Packet& packet);
  void recvPacket(SOCKET socket);
  virtual void packetHandler(SOCKET socket,const Packet& packet) = 0;
};

#endif // DATAHANDLER_H
