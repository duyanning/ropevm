/* Macros for reading data values from class files - values
   are in big endian format, and non-aligned.  See arch.h
   for READ_DBL - this is platform dependent */

#define READ_U1(v,p,l)  v = *(p)++
#define READ_U2(v,p,l)  v = ((p)[0]<<8)|(p)[1]; (p)+=2
#define READ_U4(v,p,l)  v = ((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|(p)[3]; (p)+=4
#define READ_U8(v,p,l)  v = ((u8)(p)[0]<<56)|((u8)(p)[1]<<48)|((u8)(p)[2]<<40) \
                            |((u8)(p)[3]<<32)|((u8)(p)[4]<<24)|((u8)(p)[5]<<16) \
                            |((u8)(p)[6]<<8)|(u8)(p)[7]; (p)+=8

#define READ_INDEX(v,p,l)               READ_U2(v,p,l)
#define READ_TYPE_INDEX(v,cp,t,p,l)     READ_U2(v,p,l)

/* The default value of the boot classpath is based on the JamVM
   and Classpath install directories.  If zip support is enabled
   the classes will be contained in ZIP files, else they will be
   separate class files in a directory structure */

#ifdef USE_ZIP
#define JAMVM_CLASSES INSTALL_DIR"/share/jamvm/classes.zip"
#define CLASSPATH_CLASSES CLASSPATH_INSTALL_DIR"/share/classpath/glibj.zip"
#else
//#define JAMVM_CLASSES INSTALL_DIR"/share/jamvm/classes"
#define JAMVM_CLASSES INSTALL_DIR"/lib/classes"
#define CLASSPATH_CLASSES CLASSPATH_INSTALL_DIR"/share/classpath"
#endif

#define DFLT_BCP JAMVM_CLASSES ":" CLASSPATH_CLASSES
