#include "xmlreader.h"
#include <QFile>


std::optional<QDomDocument> XmlReader::read(const QString &path)
{
  QDomDocument xmlDoc;

  QFile file(path);

  if(!file.open(QIODevice::ReadOnly)){
    return std::nullopt;
  }

  if(!xmlDoc.setContent(&file)){
    file.close();
    return std::nullopt;
  }

  file.close();
  return xmlDoc;
}
