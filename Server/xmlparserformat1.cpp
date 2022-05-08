#include "xmlparserformat1.h"
#include <QByteArray>
#include <QTextCodec>


XmlData XmlParserFormat1::parse(const QDomDocument &doc)
{

  XmlData data;

  std::vector<std::pair<QString,QByteArray>> result;

  std::vector<QString> attributes;
  attributes.push_back("from");
  attributes.push_back("to");
  attributes.push_back("FormatVersion");
  QDomElement messageElem = doc.firstChildElement();
  retrievElements(messageElem, "Message", attributes,result);


  attributes.clear();
  attributes.push_back("id");
  QDomElement msgElem  = messageElem.firstChild().toElement();
  retrievElements(msgElem,"msg",attributes,result);


  QDomElement textElem = msgElem.firstChild().toElement();
  attributes.clear();
  attributes.push_back("color");
  attributes.push_back("text");
  retrievElements(textElem,"text",attributes,result);


  QDomElement imageElem = msgElem.firstChild().toElement();
  attributes.clear();
  attributes.push_back("image");
  retrievElements(imageElem,"image",attributes,result);

  data.setData(std::move(result));

  return data;
}
