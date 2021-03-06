typedef struct zip_file {
    int length;
    unsigned char *data;
    HashTable *dir_hash;
} ZipFile;

extern ZipFile *processArchive(char *path);
extern char *findArchiveDirEntry(char *pathname, ZipFile *zip);
extern char *findArchiveEntry(char *pathname, ZipFile *zip, int *entry_len);
