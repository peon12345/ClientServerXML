#ifndef XMLPARSERFORMAT1_H
#define XMLPARSERFORMAT1_H
#include <xmlparser.h>

class XmlParserFormat1 : public XmlParser
{
public:
  XmlParserFormat1() = default;
  ~XmlParserFormat1() override = default;
  XmlData parse(const QDomDocument& doc) override;
};

#endif // XMLPARSERFORMAT1_H
