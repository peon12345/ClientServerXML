#include "datastruct.h"



PacketInfo::PacketInfo() : m_typePacket (TypePacket::UNKNOWN),
  m_typeDataAcces(TypeDataAccess::PRIVATE_DATA),
  countReceiver(0),
  sizeMetaData(0)

{
}

bool PacketInfo::isValid()
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

PacketInfo::operator char *()
{

}

void PacketInfo::setTypePacket(TypePacket typePacket)
{
  m_typePacket = typePacket;
}

void PacketInfo::setTypeDataAcces(TypeDataAccess typeAcces)
{
  m_typeDataAcces = typeAcces;
}

void PacketInfo::setSize(const std::array<uint8_t, 4> &size)
{
  m_sizeData = size;
}

void PacketInfo::setSize(const std::vector<char>& size)
{
  for(size_t i = 0; i <size.size(); ++i){
      m_sizeData.at(i) = size.at(i);
    }
}
