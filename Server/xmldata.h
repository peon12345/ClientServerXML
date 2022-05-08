#ifndef XMLDATA_H
#define XMLDATA_H
#include <list>
#include <QString>
#include <vector>
#include <optional>

#include <unordered_map>



struct HasherQString
{
  std::size_t operator()(const QString& s) const noexcept
  {
     return std::hash<std::string>{}(s.toStdString());
  }
};



class XmlData final
{
public:
  XmlData() = default;
  XmlData(const XmlData& data) = default;
  XmlData(XmlData&& data) = default;
  XmlData& operator= (const XmlData& data) = default;
  XmlData& operator= (XmlData&&) = default;
  ~XmlData() = default;


  explicit operator bool() const;

  std::list<QByteArray> values(const QString& attributeName);
  std::list<std::pair<QString,QByteArray>> values(const std::vector<QString>& attributeNames);

  void setData(std::vector<std::pair<QString,QByteArray>>&& data);

private:
  std::unordered_multimap<QString,QByteArray,HasherQString> m_data;
};

#endif // XMLDATA_H
