 
typedef struct HeaderBlock
{
    char keyType; /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
    int keySize; /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
    char secFieldType; /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
    int secFieldSize; /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
    int ptrSize;
    int isEmpty;
    int NumOfKeysInIndexBlock;
    int NumOfRecsInDataBlock;
    int root;
}HeaderBlock;


typedef struct DataBlockInfo{
    int isIndexBlock;
    int CurrNumOfValues;
    int parentBlock;
    int nextBlock;
}DataBlockInfo;

typedef struct IndexBlockInfo{
    int isIndexBlock;
    int CurrNumOfKeys;
    int parentBlock;
}IndexBlockInfo;

typedef struct FileRecord{
    int fileDesc;
    char * fileName;
}FileRecord;


typedef struct ScanRecord{
    int fileDesc;
    int dataBlock;
    int op;
    int currEntryPos;
    void * key;
}ScanRecord;