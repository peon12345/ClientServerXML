#ifndef DATASTRUCT_H
#define DATASTRUCT_H
#include <string>
#include <vector>
#include <winsock2.h>
#include <uchar.h>
#include <algorithm>
#include <array>
#include <QString>

enum class TypePacket : int8_t {
  INFO_CLIENT,
  MESSAGE,
  IMAGE,
  UNKNOWN,
};

enum class TypeDataAccess : int8_t {
  PUBLIC_DATA,
  PRIVATE_DATA,
};


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


class PacketHeader {
public:
  PacketHeader();

 explicit PacketHeader(const std::vector<char>& headerData);
 virtual ~PacketHeader() = default;

 void setHeaderData(const std::vector<char>& headerData);
 void setTypePacket(TypePacket typePacket);
 void setTypeDataAcces(TypeDataAccess typeAcces);
 void setSize(const std::array<uint8_t,4>& size);
 void setSize(const std::vector<char>& size);
 void setSize(int size);
 void setMetaData(const std::vector<char>& metaData);
 void setReceivers(const std::vector<QString>& receivers );
 void appendReceiver(const QString& name );
 bool isValid() const;

 virtual std::vector<char> convertToVector() const;

 static uint8_t sizeHeader(TypePacket type, TypeDataAccess typeAccess = TypeDataAccess::PUBLIC_DATA, int countReceivers = 0);

 TypePacket type() const;
 TypeDataAccess typeAccess() const;
public:
 static constexpr int LEN_TYPE_PACKET = 1;
 static constexpr int LEN_TYPE_DATA_ACCESS = 1;
 static constexpr int LEN_COUNT_NAME = 1;
 static constexpr int LEN_SIZE_DATA_INFO = 4;
 static constexpr int LEN_FORMAT_IMAGE = 3;

private:
  TypePacket m_typePacket;
  TypeDataAccess m_typeDataAcces;
  std::array<uint8_t,4> m_sizeData;

  uint8_t m_countReceiver;
  std::vector<QString> m_receivers;

  uint8_t m_sizeMetaData;
  std::vector<char> m_metaData;
};


class Packet : public PacketHeader {

public:
  Packet() = default;
  explicit Packet(const std::vector<char>& dataWithHeader);

  void setDataWithHeader(const std::vector<char>& dataWithHeader);
  std::vector<char> convertToVector() const override;

  void setData(const std::vector<char>& data);
  void setData(std::vector<char>&& data);
private:
  std::vector<char> m_data;
};


namespace Data {

  inline bool fillHeader(std::vector<char>& data,TypePacket type,
                         int size = -1 , TypeDataAccess typeAccess = TypeDataAccess::PUBLIC_DATA , const std::vector<QString>& receivers = std::vector<QString>()){

   PacketHeader packetHeader;

   switch (type) {

     case TypePacket::INFO_CLIENT:{
       packetHeader.setTypePacket(type);
       break;
       }

     case TypePacket::MESSAGE:
     case TypePacket::IMAGE:{
         packetHeader.setTypePacket(type);
         packetHeader.setTypeDataAcces(typeAccess);
         packetHeader.setSize(size);

         if(type == TypePacket::IMAGE){
         static std::vector<char> formatImage {'b','m','p'};

         packetHeader.setMetaData(formatImage);
         }
       }

      case TypePacket::UNKNOWN:{

       return false;
       }

     }

   switch (typeAccess) {

     case TypeDataAccess::PRIVATE_DATA:{

       if(!receivers.empty()){
         packetHeader.setReceivers(receivers);

         }
       }

     default: {

       break;
       }

     }



   if(!packetHeader.isValid()){
     return false;
     }

   std::vector<char> temp = packetHeader.convertToVector();
   data.insert(data.begin(),temp.begin(),temp.end());

   return true;

  }

  inline size_t accumulateSize(const std::vector<char>& sizeBuf){
    size_t size =0;
    std::for_each(sizeBuf.begin(), sizeBuf.end(), [&size](const char c){ size += static_cast<unsigned char>(c); });
    return size;
  }


}





#endif // DATASTRUCT_H
