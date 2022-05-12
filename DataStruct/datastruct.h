#ifndef DATASTRUCT_H
#define DATASTRUCT_H
#include <string>
#include <vector>
#include <winsock2.h>
#include <uchar.h>
#include <algorithm>
#include <QString>
#include <array>

enum class TypePacket : int8_t {
  INFO_CLIENT,
  MESSAGE,
  IMAGE,
  COMMAND,
  UNKNOWN,
};

enum class TypeDataAccess : int8_t {
  PUBLIC_DATA,
  PRIVATE_DATA,
  UNKNOWN
};


class ClientInfo final {
public:
  ClientInfo();
  ClientInfo(const std::string& name);

  ~ClientInfo() = default;

  void setName(const std::string& name);

  std::string getName() const;


public:
  static constexpr size_t MAX_LENGHT_NAME = 24;
private:

  uint8_t m_lenName;
  std::array<char,MAX_LENGHT_NAME> m_name;
};


class PacketHeader {
public:
  PacketHeader();

 explicit PacketHeader(const std::vector<char>& headerData);
 virtual ~PacketHeader() = default;

 virtual std::vector<char> convertToVector() const;

 static uint8_t sizeHeader(TypePacket type, TypeDataAccess typeAccess = TypeDataAccess::PUBLIC_DATA, int countReceivers = 0);
 static size_t sizePacketBuffLen(TypePacket type);
 TypePacket type() const;
 TypeDataAccess typeAccess() const;
public:
 static constexpr int LEN_TYPE_PACKET = 1;
 static constexpr int LEN_TYPE_DATA_ACCESS = 1;
 static constexpr int LEN_COUNT_NAME = 1;
 static constexpr int LEN_FORMAT_IMAGE = 3;

 static constexpr int LEN_SIZE_MESSAGE = 100;
 static constexpr int LEN_SIZE_IMAGE = 200;
 public:

 void setHeaderData(const std::vector<char>& headerData);
 void setTypePacket(TypePacket typePacket);
 void setTypeDataAccess(TypeDataAccess typeAccess);
 void setSize(const std::vector<char>& size,bool withHeader = false);
 void setSize(int size,bool withHeader = false);
 void setMetaData(const std::vector<char>& metaData);
 void setReceivers(const std::vector<QString>& receivers );
 void appendReceiver(const QString& name );
 bool isValid() const;

 const std::vector<QString>& getReceivers() const ;
 const std::vector<char>& getMetaData() const ;

private:
  TypePacket m_typePacket;
  TypeDataAccess m_typeDataAcces;
  std::vector<char> m_sizeData;

  uint8_t m_countReceiver;
  std::vector<QString> m_receivers;

  uint8_t m_sizeMetaData;
  std::vector<char> m_metaData;
protected:
  int m_offset;
};


class Packet : public PacketHeader {

public:
  Packet() = default;
  ~Packet() override = default;
  explicit Packet(const std::vector<char>& dataWithHeader);

  void setDataWithHeader(const std::vector<char>& dataWithHeader);
  std::vector<char> convertToVector() const override;

  void setData(const std::vector<char>& data);
  void setData(const QByteArray& array);
  void setData(std::vector<char>&& data);

  const std::vector<char> &getData() const;

private:
  std::vector<char> m_data;
};





#endif // DATASTRUCT_H
