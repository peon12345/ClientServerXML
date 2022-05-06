#ifndef DATASTRUCT_H
#define DATASTRUCT_H
#include <string>
#include <vector>
#include <winsock2.h>
#include <uchar.h>
#include <algorithm>

enum class TypePacket : int8_t {
  INFO_CLIENT,
  DATA,
  PRIVATE_DATA,
  UNKNOWN,
};

inline constexpr int LEN_TYPE_PACKET = 1;
inline constexpr int LEN_COUNT_NAME = 1;
inline constexpr int LEN_SIZE_DATA_INFO = 4;

namespace Data {

inline uint8_t sizeHeader(TypePacket type){

  switch(type) {
  case TypePacket::INFO_CLIENT: {
    return LEN_TYPE_PACKET;
    break;
  }

  case TypePacket::DATA: {
    return LEN_TYPE_PACKET + LEN_SIZE_DATA_INFO;
    break;
  }

  case TypePacket::PRIVATE_DATA: {

    return LEN_TYPE_PACKET +  LEN_COUNT_NAME +  LEN_SIZE_DATA_INFO;
  }

  default:{

    return  LEN_TYPE_PACKET + LEN_SIZE_DATA_INFO;
  }

  }
}


inline void fillHeader(std::vector<char>& data,TypePacket type,int size = -1){


  int index = 0;
  if(data.empty()){
  data.resize(sizeHeader(type));
  }

  data.at(index++) = static_cast<char>(type);

  switch (type) {

  case TypePacket::PRIVATE_DATA:
  case TypePacket::DATA: { //добавляем информацию о длине
    if(size >0){

      if(data.size() < LEN_SIZE_DATA_INFO){
        data.resize(data.size() + (LEN_SIZE_DATA_INFO - data.size()) );
      }

      for(int i = 0;i< LEN_SIZE_DATA_INFO;++i){

        if(size > 255){
          data.at(index++) = -1;
          size -= 255;
        }else if(size > 0){
          data.at(index++) = size;
          size -= 255;
        } else{
          data.at(index++) = 0;
        }
      }
    }
    break;
  }

  default:
    break;
  }
}

inline size_t accumulateSize(const std::vector<char>& sizeBuf){
  size_t size =0;
  std::for_each(sizeBuf.begin(), sizeBuf.end(), [&size](const char c){ size += static_cast<unsigned char>(c); });
  return size;
}

inline bool recvData(SOCKET socket ,std::vector<char>& dataOutput , size_t size){

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

}


class ClientInfo final {
public:
  ClientInfo()  {
  setName("Anonim Anonimovich");

  }
  ClientInfo(const std::string& name){
    setName(name);
  }

  ~ClientInfo() = default;

  void setName(const std::string& name){

    strcpy(m_name,name.c_str());
    memset(m_name + name.length(), '~', MAX_LENGHT_NAME);
    m_lenName = name.length();
  }

  std::string getName() const{
    return std::string(m_name, m_lenName);
  }

public:
   static constexpr size_t MAX_LENGHT_NAME = 24;
private:

  uint8_t m_lenName;
  char m_name[MAX_LENGHT_NAME];
};

#endif // DATASTRUCT_H
