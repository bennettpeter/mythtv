#ifndef FILEOBS_H_
#define FILEOBS_H_
/*
	fileobs.h

	(c) 2003 Thor Sigvaldason and Isaac Richards
	Part of the mythTV project
	
	Headers for file objects that know
	how to do clever things

*/

#include <qstring.h>
#include <qfile.h>
#include <qptrlist.h>

class RipFile
{


  public:
  
    RipFile(const QString &a_base, const QString &an_extension);
    ~RipFile();
    
    bool    open(int mode);
    void    close();
    void    remove();
    QString name();
    bool    writeBlocks(unsigned char *the_data, int how_much);

  private:
  
    QString         base_name;
    QString         extension;
    int             filesize;
    QFile           *active_file;
    int             bytes_written;
    int             access_mode;
    QPtrList<QFile> files;
};

#endif  // fileobs_h_

