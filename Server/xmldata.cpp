#include "xmldata.h"



XmlData::operator bool() const
{
  return !m_data.empty();
}

std::list<std::pair<QString, QByteArray> > XmlData::values(const std::vector<QString>& attributeNames)
{

  std::list<std::pair<QString,QByteArray>> result;

  for(const QString& attribute : attributeNames){

    std::list<QByteArray> attributeValues = values(attribute);

    if(!attributeValues.empty()){
      for(const QByteArray& v : attributeValues){
        result.push_back(std::make_pair(attribute,v));
      }
    }
  }
  return result;
}


void XmlData::setData(std::vector<std::pair<QString,QByteArray>>&& data)
{

  for(std::pair<QString,QByteArray>& pair : data){
    m_data.insert(std::move(pair));
  }

  data.clear();


}


std::list<QByteArray> XmlData::values(const QString &attributeName)
{

  std::list<QByteArray> values;
  auto range = m_data.equal_range(attributeName);

  if(range.first == range.second){
    return values;
  }


  for_each (
      range.first,
      range.second,
      [&values](const std::pair<QString,QByteArray>& pair){

        values.push_back(pair.second);
      }
      );

  return values;

}
