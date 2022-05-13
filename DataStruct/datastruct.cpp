#include "datastruct.h"



PacketHeader::PacketHeader() : m_typePacket (TypePacket::UNKNOWN),m_typeDataAcces(TypeDataAccess::UNKNOWN),
                               m_countReceiver(0),
                               m_sizeMetaData(0),
                               m_offset(0)

{
}

void PacketHeader::setHeaderData(const std::vector<char> &headerData)
{
  std::vector<char>::const_iterator it;

  if(!headerData.empty()){

    it = headerData.begin();

    if(m_typePacket == TypePacket::UNKNOWN){
      m_typePacket = static_cast<TypePacket>(*it++);
    }

    if(TypePacket::COMMAND == m_typePacket){
      m_metaData.push_back(*it++);
      return void();
    }

    if(m_typePacket != TypePacket::INFO_CLIENT){

      if(m_sizeData.empty()) {
        size_t size = sizePacketBuffLen(m_typePacket);

        for(size_t i = 0;i<size;++i){
          m_sizeData.push_back(*it++);
        }
      }


      if(m_typeDataAcces  == TypeDataAccess::UNKNOWN){
        m_typeDataAcces = static_cast<TypeDataAccess>(*it++);

        if(m_typeDataAcces != TypeDataAccess::PUBLIC_DATA){
          m_offset += 2;
        }
      }

    }else{
      m_offset +=1;
      return void();
    }

    switch (m_typeDataAcces) {

    case TypeDataAccess::PRIVATE_DATA:{
      if(m_receivers.empty()){
        m_countReceiver = *it++;

        for(int i = 0; i < m_countReceiver; ++i){
          std::string name;

          for(size_t j = 0; j < ClientInfo::MAX_LENGHT_NAME; ++j){
            name += *it++;
          }
          m_receivers.push_back(QString::fromLocal8Bit(name.c_str()));
          m_offset += ClientInfo::MAX_LENGHT_NAME;
        }
      }
    }

    default:{

      break;
    }
    }


    switch (m_typePacket) {

    case TypePacket::IMAGE:{
      if(m_metaData.empty()){
        m_sizeMetaData = LEN_FORMAT_IMAGE;
        m_metaData.clear();
        m_metaData.insert(m_metaData.end(),it,it+LEN_FORMAT_IMAGE );
        it+=LEN_FORMAT_IMAGE;
        // m_offset += LEN_FORMAT_IMAGE;
      }
    }
    default:{
      break;
    }
    }

    if(m_sizeData.empty()){
      setSize(headerData.size() - sizeHeader(type()));
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

    case TypePacket::COMMAND:{

      if(!m_metaData.empty()){
        return true;
      }
      break;
    }

    default:{
      return false;
    }
    }
  }else{
    return false;
  }

  return true;
}

const std::vector<QString> &PacketHeader::getReceivers() const
{
  return m_receivers;
}

const std::vector<char> &PacketHeader::getMetaData() const
{
  return m_metaData;
}

std::vector<char> PacketHeader::convertToVector() const
{
  std::vector<char> result;

  if(!isValid()){
    return result;
  }

  result.reserve(sizeHeader(m_typePacket));
  result.push_back(static_cast<char>(m_typePacket));

  if(m_typePacket == TypePacket::COMMAND){
    result.push_back(m_metaData.back());
    return result;
  }


  if(TypePacket::INFO_CLIENT != m_typePacket){
    result.insert(result.end(),m_sizeData.begin(),m_sizeData.end());
    result.back() = sizeHeader(m_typePacket,m_typeDataAcces,m_receivers.size()); // так же добавить рамзер самого заголовка
    result.push_back(static_cast<char>(m_typeDataAcces));
  }else{
    result.push_back(sizeof(ClientInfo) + sizeHeader(m_typePacket,m_typeDataAcces,m_receivers.size()));
  }


  if(TypeDataAccess::PUBLIC_DATA != m_typeDataAcces && m_typePacket != TypePacket::INFO_CLIENT){
    result.push_back(m_countReceiver);


    for(const QString& name : m_receivers){

      QByteArray arr = name.toLocal8Bit();
      std::string stdName(arr.begin(),arr.end());

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


  case TypePacket::INFO_CLIENT:{
    return result;
  }

  default:
    break;
  }

  return result;
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
    sizeHeader+= LEN_TYPE_PACKET + LEN_SIZE_MESSAGE;
    break;
  }
  case TypePacket::IMAGE: {
    sizeHeader += LEN_TYPE_PACKET + LEN_FORMAT_IMAGE + LEN_SIZE_IMAGE;
    break;
  }

  default:{

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

size_t PacketHeader::sizePacketBuffLen(TypePacket type)
{

  std::vector<char> sizeBuf;

  switch (type) {

  case TypePacket::MESSAGE:{

    return LEN_SIZE_MESSAGE;
  }

  case TypePacket::IMAGE:{

    return LEN_SIZE_IMAGE;
  }

  case TypePacket::INFO_CLIENT:{

    return 1;
    break;
  }

  default:
    return 0;
    break;
  }

}

TypePacket PacketHeader::type() const
{
  return m_typePacket;
}

TypeDataAccess PacketHeader::typeAccess() const
{
  return m_typeDataAcces;
}

void PacketHeader::setTypePacket(TypePacket typePacket)
{
  m_typePacket = typePacket;
  m_offset--;
}

void PacketHeader::setTypeDataAccess(TypeDataAccess typeAccess)
{
  m_typeDataAcces = typeAccess;
  m_offset--;
}

void PacketHeader::setSize(const std::vector<char>& size,bool withHeader)
{
  if(size.empty()){
    return void();
  }

  for(size_t i = 0; i <size.size(); ++i){
    m_sizeData.push_back(size.at(i));
    m_offset--;
  }

  if(withHeader){
    m_sizeData.back() = 0;
  }
}

void PacketHeader::setSize(int size,bool withHeader)
{

  if(m_typePacket == TypePacket::UNKNOWN){

    return void();
  }

  m_sizeData.clear();
  size_t sizeLen = sizePacketBuffLen(m_typePacket);

  if(size >0){

    for(size_t i = 0;i< sizeLen;++i){

      if(size > 255){
        m_sizeData.push_back(-1);
        m_offset--;
        size -= 255;
      }else if(size > 0){
        m_sizeData.push_back(size);
        m_offset--;
        size -= 255;
      } else{
        m_sizeData.push_back(0);
        m_offset--;
      }
    }
  }

  if(withHeader){
    m_sizeData.back() = 0;
  }

}

void PacketHeader::setMetaData(const std::vector<char> &metaData)
{
  m_sizeMetaData = metaData.size();
  m_metaData = metaData;
  m_offset -= metaData.size();
}

void PacketHeader::appendMetaData(char c)
{
  m_metaData.push_back(c);
  m_sizeMetaData++;
  m_offset--;
}

void PacketHeader::setReceivers(const std::vector<QString> &receivers)
{
  m_receivers = receivers;
  m_offset -= receivers.size() * ClientInfo::MAX_LENGHT_NAME;
  m_countReceiver = m_receivers.size();
}

void PacketHeader::appendReceiver(const QString &name)
{
  m_receivers.push_back(name);
  m_offset -= ClientInfo::MAX_LENGHT_NAME;
  m_countReceiver++;
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

  m_offset += PacketHeader::sizeHeader(type());

  std::vector<char> data(dataWithHeader.begin() + m_offset,dataWithHeader.end());
  setData(std::move(data));
}

std::vector<char> Packet:: convertToVector() const
{
  std::vector<char> result =  PacketHeader::convertToVector();

  if(!result.empty()){
    result.insert(result.end(),m_data.begin(),m_data.end());
  }

  return result;
}

void Packet::setData(const std::vector<char> &data)
{
  m_data = data;
  setSize(m_data.size());
}

void Packet::setData(const QByteArray &array)
{
  m_data.clear();
  m_data.insert(m_data.end(),array.begin(),array.end());
  setSize(m_data.size());
}

void Packet::setData(std::vector<char> &&data)
{
  m_data = std::move(data);
  setSize(m_data.size());
}

const std::vector<char>& Packet::getData() const
{
  return m_data;
}


ClientInfo::ClientInfo()
{
  setName("Anonim Anonimovich");
}

ClientInfo::ClientInfo(const std::string &name)
{
  setName(name);
}

void ClientInfo::setName(const std::string &name)
{
  if(name.length() > MAX_LENGHT_NAME){
    return void();
  }

  for(size_t i = 0;i < name.length(); ++ i){
    m_name.at(i) = name.at(i);
  }

  m_lenName = name.length();

  for(size_t i = name.length();i < (MAX_LENGHT_NAME - m_lenName); ++i){
    m_name.at(i) =  '~';
  }

}

std::string ClientInfo::getName() const
{
  return std::string  (m_name.begin(),m_name.begin() + m_lenName );
}
