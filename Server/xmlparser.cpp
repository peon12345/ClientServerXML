#include "xmlparser.h"



void XmlParser::retrievElements(const QDomElement& root,const QString& tag,const std::vector<QString>& attributes, std::vector<std::pair<QString,QByteArray>>& outputValues)
{
  //std::vector<std::pair<QString,QByteArray>> result;


  QDomNodeList nodes = root.elementsByTagName(tag);

  for(int i = 0; i < nodes.count(); i++)
  {
    QDomNode elm = nodes.at(i);
    if(elm.isElement())
    {
      QDomElement e = elm.toElement();

      for(const QString& attributeName : attributes){

        if(tag == attributeName){
          outputValues.push_back(std::make_pair( attributeName, e.text().toUtf8() ));
        }else{
          outputValues.push_back(std::make_pair( attributeName, e.attribute(attributeName).toUtf8() ));
        }
      }

    }
  }

}
