#ifndef XMLREADER_H
#define XMLREADER_H
#include <QXmlStreamReader>
#include <QDomDocument>


class XmlReader final
{
public:
  XmlReader() = delete;
  XmlReader(const XmlReader&) = delete;
  XmlReader(XmlReader&&) = delete;
  //узнать формат в методе статик?

  static std::optional<QDomDocument> read(const QString& path);
};

#endif // XMLREADER_H
