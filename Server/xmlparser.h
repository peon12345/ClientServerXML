#ifndef XMLPARSER_H
#define XMLPARSER_H
#include <list>
#include <vector>
#include <QString>
#include <QDomDocument>


class XmlParser final
{
public:
  XmlParser() = delete;
  XmlParser(const XmlParser& p) = delete;
  XmlParser(XmlParser&& p) = delete;

  enum class  XmlFormatSupport : uint8_t{
    UNIDENTIFIED = 0,
    MESSAGE_WITH_IMAGE = 1
  };


 static std::list<std::pair<QString,std::vector<char>>> parse(const QDomDocument& doc);
};

#endif // XMLPARSER_H
