#ifndef __FILECREATE_FILETYPE_H__
#define __FILECREATE_FILETYPE_H__

#include <qptrlist.h>

class FileCreateFileType {

public:

  FileCreateFileType() {
    m_subtypes.setAutoDelete(true);
  }

  void setName(const QString & name) { m_name = name; }
  QString name() const { return m_name; }
  void setExt(const QString & ext) { m_ext = ext; }
  QString ext() const { return m_ext; }
  void setCreateMethod(const QString & createMethod) { m_createMethod = createMethod; }
  QString createMethod() const { return m_createMethod; }
  void setSubtypeRef(const QString & subtypeRef) { m_subtypeRef = subtypeRef; }
  QString subtypeRef() const { return m_subtypeRef; }

  void addSubtype(const FileCreateFileType * subtype) { m_subtypes.append(subtype); }
  QPtrList<FileCreateFileType> subtypes() const { return m_subtypes; }
  
private:
  QString m_name;
  QString m_ext;
  QString m_createMethod;
  QString m_subtypeRef;
  
  QPtrList<FileCreateFileType> m_subtypes;

};

#endif
