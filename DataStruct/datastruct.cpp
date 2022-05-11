#include "datastruct.h"



PacketHeader::PacketHeader() : m_typePacket (TypePacket::UNKNOWN),
  m_typeDataAcces(TypeDataAccess::PRIVATE_DATA),
  m_countReceiver(0),
  m_sizeMetaData(0)

{
}

void PacketHeader::setHeaderData(const std::vector<char> &headerData)
{
  std::vector<char>::const_iterator it;

   if(!headerData.empty()){

   it = headerData.begin();

   m_typePacket = static_cast<TypePacket>(*it++);
   m_typeDataAcces = static_cast<TypeDataAccess>(*it++);

   switch (m_typeDataAcces) {

     case TypeDataAccess::PRIVATE_DATA:{

       m_countReceiver = *it++;

       for(int i = 0; i < m_countReceiver; ++i){
          std::string name;

           for(size_t j = 0; j < ClientInfo::MAX_LENGHT_NAME; ++j){

             name += *it++;

             }
           m_receivers.push_back(QString::fromLocal8Bit(name.c_str()));
       }

     }

     default:{

       break;
       }
   }


   switch (m_typePacket) {

     case TypePacket::IMAGE:{

         m_sizeMetaData = LEN_FORMAT_IMAGE;
         m_metaData.clear();
         m_metaData.insert(m_metaData.end(),it,it+LEN_FORMAT_IMAGE );
         it+=LEN_FORMAT_IMAGE;

       }
     default:{
     break;
     }
  }

   m_sizeData.fill(0);
   for(int i = 0; i < LEN_SIZE_DATA_INFO; ++i){
     m_sizeData.at(i) = *it++;
     }
   }
}

PacketHeader::PacketHeader(const std::vector<char> &headerData) : PacketHeader()
{
  setHeaderData(headerData);
}

bool PacketHeader::isValid() const
{
  if(m_typePacket != TypePacket::UNKNOWN){

      if(m_typeDataAcces == TypeDataAccess::PRIVATE_DATA &&
         m_receivers.empty()){
          return false;
        }

      switch (m_typePacket) {

        case TypePacket::IMAGE:{
            if(m_sizeData.empty()){
                return false;
              }
            if(m_metaData.empty()){
                return false;
              }
            break;
          }
        case TypePacket::MESSAGE:{

            if(m_sizeData.empty()){
                return false;
              }
            break;
          }
        case TypePacket::INFO_CLIENT:{

            return true;
          }
        default:{
            return false;
          }
        }
    }

  return true;
}

std::vector<char> PacketHeader::convertToVector() const
{
  std::vector<char> result;

  if(!isValid()){
      return result;
    }

  result.reserve(sizeHeader(m_typePacket));
  result.push_back(static_cast<char>(m_typePacket));
  result.push_back(static_cast<char>(m_typeDataAcces));

  if(TypeDataAccess::PUBLIC_DATA != m_typeDataAcces){
      result.push_back(m_countReceiver);


      for(const QString& name : m_receivers){

          std::string stdName =  name.toStdString();

          result.insert(result.end(),  stdName.begin(),  stdName.end());
          int diff =  ClientInfo::MAX_LENGHT_NAME - name.length();

          if(diff > 0){
              for(int i =0; i < diff; ++i){
                  result.push_back('~');
                }
            }
        }
    }

  switch (m_typePacket) {
    case TypePacket::IMAGE:{
        result.insert(result.end(),m_metaData.begin(),m_metaData.end());
        break;
      }

    default:
      break;
    }

  result.insert(result.end(),m_sizeData.begin(),m_sizeData.end());
}

uint8_t PacketHeader::sizeHeader(TypePacket type, TypeDataAccess typeAccess, int countReceivers)
{

  uint8_t sizeHeader = 0;

  switch(type) {
    case TypePacket::INFO_CLIENT: {
        sizeHeader += LEN_TYPE_PACKET;
        break;
      }

    case TypePacket::MESSAGE: {
        sizeHeader+= LEN_TYPE_PACKET + LEN_SIZE_DATA_INFO;
        break;
      }
    case TypePacket::IMAGE: {
        sizeHeader += LEN_TYPE_PACKET + LEN_FORMAT_IMAGE + LEN_SIZE_DATA_INFO;
        break;
      }

    default:{

        sizeHeader+= LEN_TYPE_PACKET + LEN_SIZE_DATA_INFO;
        break;
      }
    }

  switch (typeAccess) {
    case TypeDataAccess::PRIVATE_DATA:{

        sizeHeader += countReceivers * ClientInfo::MAX_LENGHT_NAME;

      }

    default:
      break;
    }

  return sizeHeader;
}

TypePacket PacketHeader::type() const
{
  return m_typePacket;
}



void PacketHeader::setTypePacket(TypePacket typePacket)
{
  m_typePacket = typePacket;
}

void PacketHeader::setTypeDataAcces(TypeDataAccess typeAcces)
{
  m_typeDataAcces = typeAcces;
}

void PacketHeader::setSize(const std::array<uint8_t, 4> &size)
{
  m_sizeData = size;
}

void PacketHeader::setSize(const std::vector<char>& size)
{
  for(size_t i = 0; i <size.size(); ++i){
      m_sizeData.at(i) = size.at(i);
    }
}

void PacketHeader::setSize(int size)
{
  if(size >0){

      for(int i = 0;i< LEN_SIZE_DATA_INFO;++i){

          if(size > 255){
              m_sizeData.at(i) = -1;
              size -= 255;
            }else if(size > 0){
              m_sizeData.at(i) = size;
              size -= 255;
            } else{
              m_sizeData.at(i) = 0;
            }
        }
    }
}

void PacketHeader::setMetaData(const std::vector<char> &metaData)
{
  m_sizeMetaData = metaData.size();
  m_metaData = metaData;

}

void PacketHeader::setReceivers(const std::vector<QString> &receivers)
{
  m_receivers = receivers;
}


Packet::Packet(const std::vector<char> &dataWithHeader) : PacketHeader(dataWithHeader)
{
  setDataWithHeader(dataWithHeader);
}

void Packet::setDataWithHeader(const std::vector<char> &dataWithHeader)
{
   PacketHeader::setHeaderData(dataWithHeader);

   if(type() == TypePacket::UNKNOWN){
     return void();
     }

   int offset = PacketHeader::sizeHeader(type());

   std::vector<char> data(dataWithHeader.begin() + offset,dataWithHeader.end());
   setData(std::move(data));
}

std::vector<char> Packet::convertToVector() const
{
  std::vector<char> result =  PacketHeader::convertToVector();

  if(result.empty()){
    result.insert(result.end(),m_data.begin(),m_data.end());
   }
}

void Packet::setData(const std::vector<char> &data)
{
  m_data = data;
}

void Packet::setData(std::vector<char> &&data)
{
  m_data = std::move(data);
}
