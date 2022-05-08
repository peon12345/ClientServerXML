#ifndef XMLPARSER_H
#define XMLPARSER_H
#include <list>
#include <vector>
#include <QString>
#include <QDomDocument>
#include <xmldata.h>


class XmlParser
{
public:
  XmlParser() = default;
  virtual ~XmlParser() = default;


  XmlParser& operator = (const XmlParser&) = delete;
  XmlParser& operator = (XmlParser&&) = delete;

  XmlParser(const XmlParser& p) = delete;
  XmlParser(XmlParser&& p) = delete;

  enum class  XmlFormatSupport : uint8_t{
    UNIDENTIFIED = 0,
    MESSAGE_WITH_IMAGE = 1
  };

 virtual XmlData parse(const QDomDocument& doc) = 0;

protected:
 void retrievElements(const QDomElement& root, const QString& tag, const std::vector<QString>& attributes, std::vector<std::pair<QString, QByteArray> > &outputValues);
};

#endif // XMLPARSER_H
