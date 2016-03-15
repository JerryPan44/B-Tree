#include "AM.h"
#include "BF/BF.h"
#include "Metadata.h"
#include "stdio.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define DATA_BLOCK 1
#define INDEX_BLOCK 0
#define MAXOPENFILES 20
#define MAXSCANS 20
#define ROOT 1
#define PARENT_OFFSET 2

int Insert(int fd, HeaderBlock * headerBlock, void * keyField, void * secField);
int insertIntoParent(int fd, int parent, int left, void * key, int right, HeaderBlock * headerBlock);
int insertIntoNewRoot(int fd, int left, void * key, int right, HeaderBlock * headerBlock);

/*
 * Print the key given the its type
 */
void PrintKey(char keyType, int keySize, void * key)
{
    if(keyType == 'i')						//if key is an integer
    {
        int k;
        memcpy(&k, key, keySize);				//copy the key on the k's pointer
        printf(" %d\n", k);					//print k variable
    }
    if(keyType == 'f')						//if key is a real number
    {
        float k;
        memcpy(&k, key, keySize);
        printf(" %f\n", k);
    }
    if(keyType == 'c')						//if key is a string
    {	
        char k[keySize];
        memcpy(k, key, keySize);
        printf(" %s\n",k);
    }
}

/*
 * Decide if two given values are equal
 */
int areEqual(char type,int size, void * v1, void * v2)
{
    if(type == 'i')						//if the values are integers
    {
        int value1, value2;
        memcpy(&value1, v1, sizeof(int));			//copy v1 on integer value1
        memcpy(&value2, v2, sizeof(int));			//copy v2 on integer value2
        return value1 == value2;				//return true if v1==v2, else return false 
    }
    if(type == 'c')						//if the values are strings
    {
        char  k1[size], k2[size];
        memcpy(&k1, v1, size);					//copy v1 on string k1
        memcpy(&k2, v2, size);					//copy v2 on string k2
        return !strcmp(k1, k2);					//if k1==k2 strcmp returns 0, so if k1==k2 !strcmp(k1,k2) returns <>0, else returns 0
    }
    if(type == 'f')						//if the values are real numbers
    {
        float  k1, k2;
        memcpy(&k1, v1, size);					//copy v1 on float k1
        memcpy(&k2, v2, size);					//copy v2 on float k2
        return k1 == k2;
    }
}

/*
 * Check if variable type and length of variable are proper
 */
int checkValidityOfAttributes(char type, int length)
{
    if(type == 'i' && length == sizeof(int))			//if type is integer and length equals to the size of an integer
        return 1;
    if(type == 'f' && length == sizeof(float))			//if type is real and length equals to the size of a float
        return 1;
    if(type == 'c' && length < 255 && length > 1)
        return 1;
    //so on
    return 0;
}

/*
 * Print that the error was BF
 */
int BF_Error()
{
    BF_PrintError("");
    AM_errno = AME_BF;
    return AME_BF;
}

/*
 * Allocate given number of blocks in fd
 */
int allocateMany(int fd, int noBlocks)
{
    int i;
    for(i=0;i<noBlocks;i++)					//for all blocks in fd
    {
        if(BF_AllocateBlock(fd)!=0)				//if BF_AllocateBlock fails(returns <>0)
        {
            //BF_PrintError("");
            return BF_Error();					//return bf error if FAIL
        }
    }
    return 0;							//return 0 if successful
}

/*
 * Return the key position of Data Block
 */
int getDataBlockKeyPos(int i, int keySize, int secFieldSize)
{
    return sizeof(DataBlockInfo) + i * keySize + i * secFieldSize;
}

/*
 * Return the second field position of Data Block
 */
int getDataBlockSecFieldPos(int i, int keySize, int secFieldSize)
{
    return sizeof(DataBlockInfo) + (i + 1) * keySize + (i) * secFieldSize;
}

/*
 * Return the index key position of Data Block
 */
int getIndexKeyPos(int i, int keySize)
{
    return sizeof(IndexBlockInfo) + i * keySize + (i + 1) * sizeof(int);
}

/*
 * Return the index pointer position of Data Block
 */
int getIndexPtrPos(int i, int keySize)
{
    return sizeof(IndexBlockInfo) + i * keySize + i * sizeof(int);
}

/*
 * The list of maximum simultaneous opened files
 */
FileRecord * globalFileArray;
int currPosOfArray = 0;

/*
 * The list of maximum simultaneous file scans
 */
ScanRecord * globalFileScansArray;
int currPosOfScansArray = 0;

/*
 * Initialize AM level by allocating needed memory for globalFileArray and globalFileScansArray
 */
void AM_Init()
{
    BF_Init();								//Initialize BF level
    globalFileArray = malloc(sizeof(FileRecord)*MAXOPENFILES);		//MAXOPENFILES is defined 20
    currPosOfArray = 0;
    globalFileScansArray = malloc(sizeof(ScanRecord)*MAXSCANS);		//MAXSCANS is defined 20
    currPosOfScansArray = 0;
}

/*
 * Initialize the header in file
 */
int initIndex(int fd, int attrLength1, char attrType1, int attrLength2, char attrType2)
{
    if(allocateMany(fd, 1) < 0)
        return BF_Error();
    void *block;
    HeaderBlock hBlock;
    hBlock.keySize = attrLength1;			//
    hBlock.keyType = attrType1;				//Set Given info in header block
    hBlock.secFieldSize = attrLength2;			//
    hBlock.secFieldType = attrType2;			//
    hBlock.ptrSize = sizeof(int);			//
    hBlock.isEmpty = 1;					//
    hBlock.NumOfKeysInIndexBlock = (BLOCK_SIZE - sizeof(IndexBlockInfo) - hBlock.ptrSize)/(hBlock.keySize + hBlock.ptrSize);
    hBlock.NumOfRecsInDataBlock = (BLOCK_SIZE - sizeof(DataBlockInfo))/(hBlock.keySize + hBlock.secFieldSize);
//    hBlock.root = ROOT;					//the root of the tree
    /*
     * Insert the previously set info in the first block or return the BF error if any BF function fails (read,write)
     */
    if(BF_ReadBlock(fd, 0, &block) != 0)
        return BF_Error();

    memcpy(block, &hBlock, sizeof(hBlock));

    if(BF_WriteBlock(fd, 0) != 0)
        return BF_Error();
    return 0;

//    /*
//     * Insert parent block pointer info and the number of keys info set info in the second block or return the BF error if any BF function fails (read,write)
//     */
//    DataBlockInfo dataBlockInfo;
//    dataBlockInfo.parentBlock = -1;
//    dataBlockInfo.isIndexBlock = 0;
//    dataBlockInfo.CurrNumOfValues = 0;
//    if(BF_ReadBlock(fd, 1, &block) != 0)
//        return BF_Error();
//    memcpy(block, &dataBlockInfo, sizeof(DataBlockInfo));
//
//
//    if(BF_WriteBlock(fd, 1) == -1)
//        return BF_Error();
//
//    /*
//     * Insert the initialized info in the second block or return the BF error if any BF function fails (read,write)
//     */
//    DataBlockInfo dataBlockInfo;
//    dataBlockInfo.parentBlock = 1;
//    dataBlockInfo.CurrNumOfValues = 0;
//    dataBlockInfo.nextBlock = 3;			//we are in 2, so 3 is next
//    if(BF_ReadBlock(fd, 2, &block) != 0)
//        return BF_Error();
//    memcpy(block, &dataBlockInfo, sizeof(DataBlockInfo));
//    if(BF_WriteBlock(fd, 2) == -1)
//        return BF_Error();
//
//    /*
//     * Insert the previously set info in the first block or return the BF error if any BF function fails (read,write)
//     */
//    dataBlockInfo.parentBlock = 1;
//    dataBlockInfo.CurrNumOfValues = 0;
//    dataBlockInfo.nextBlock = -1;			//the 4th block (with pointer 3) is the last so there is no next block
//    if(BF_ReadBlock(fd, 3, &block) != 0)
//        return BF_Error();
//    memcpy(block, &dataBlockInfo, sizeof(DataBlockInfo));
//    if(BF_WriteBlock(fd, 3) == -1)
//        return BF_Error();
}

/*
 * Create a new non-existing file based on B+ tree
 */
int AM_CreateIndex(
        char *fileName,									//name of file
        char attrType1,
        int attrLength1,
        char attrType2,
        int attrLength2
)
{
    if (access(fileName, F_OK) == 0)        //if file already exists
    {
//        perror("Error : File already exists");
        AM_errno = AME_FILE_EXISTS;
        return AME_FILE_EXISTS;
    }
    int fd;
    if(!checkValidityOfAttributes(attrType1, attrLength1) || !checkValidityOfAttributes(attrType2, attrLength2))//Check if type and length of attritubes are proper
    {
        AM_errno=AME_CREATEINDEXFAILS;
        return AME_CREATEINDEXFAILS;							//return error
    }
    if(BF_CreateFile(fileName) != 0)							//create file in BF level
        return BF_Error();								//if BF_CreateFile is not successful, return error is BF
    fd = BF_OpenFile(fileName);								//Open BF file in fd
    if(fd < 0)										//if BF_OpenFile fails (has returned <0) return error
        return BF_Error();								//Print error is BF
    if(initIndex(fd, attrLength1, attrType1, attrLength2, attrType2) != 0)		//allocate header
        return BF_Error();								//Print error is BF
    if(BF_CloseFile(fd)!=0)								//Close the BF_file and return error if failed
        return BF_Error();								//Print error is BF

    return AME_OK;									//return AME_OK (is defined as 0)
}

/*
 * Delete a file that is not active (open) anywhere
 */
int AM_DestroyIndex(
        char *fileName									//name of file
)
{											//delete file
    int i;
    for (i = 0; i < currPosOfArray; ++i) {
        if(!strcmp(globalFileArray[i].fileName, fileName))				//Ensure that the file is not active (not open on the list)
        {
            AM_errno=AME_FILEISOPEN;							//return error if file is open
	        return AME_FILEISOPEN;
        }
    }
    if( remove(fileName) != 0 )								//remove the file
    {
        AM_errno=AME_DESTROYINDEXFAILS;
	    return AME_DESTROYINDEXFAILS;
    }
    return AME_OK;									//Deleting the file fails, return error, else AME_OK == 0
}

/*
 * Open a file with given filename
 */
int AM_OpenIndex (
        char *fileName									//name of file
)
{
    int fd;
    if(currPosOfArray >= MAXOPENFILES)							//Are there the maximum files open? 
    {
        AM_errno=AME_MAXFILESOPEN;
	    return AME_MAXFILESOPEN;
    }
    if((fd = BF_OpenFile(fileName))<0)							//Open the "filename" file and check if it fails
        return BF_Error();								//Print error is BF

    globalFileArray[currPosOfArray].fileDesc = fd;					//Add fd in the list of open files
    globalFileArray[currPosOfArray].fileName = malloc(sizeof(char)*(strlen(fileName) + 1));	//Allocate the needed memory space for the filename
    strcpy(globalFileArray[currPosOfArray].fileName, fileName);				//And add the name in the list of names
    currPosOfArray++;									//add 1 in currPosOfArray

    return currPosOfArray - 1;								//return the number of opened file (fileDesc)
}

/*
 * Close an opened file
 */
int AM_CloseIndex (
        int fileDesc
)
{
    int i, j;
    for(j = 0;j < currPosOfScansArray; ++j){						//Search if the file is open on scans list
        if(globalFileScansArray[j].fileDesc == fileDesc){			//Fail if there is one
            AM_errno=AME_FILESCANNING;
            return AM_errno;						//Error:File is open on scans and cannot be closed
        }
    }
    for (i = 0; i < currPosOfArray; ++i) {
        if(globalFileArray[i].fileDesc == fileDesc)					//Is fileDesc in the list of opened files
        {
            if(BF_CloseFile(fileDesc) != 0)						//close the file in BF level and check if it fails
                return BF_Error();							//Print error is BF
            for (j = i; j < currPosOfArray - 1; ++j) {					//move every file after fileDesc move one position backwards on the list
                globalFileArray[j] = globalFileArray[j + 1];
            }
            currPosOfArray--;								//Total numbers of opened files - 1
            //prostheto error an yparxoun anoixtes sarwseis

            return AME_OK;								//return OK if success
        }
    }
    AM_errno=AME_CLOSEINDEXFAILS;
    return AME_CLOSEINDEXFAILS;								//return error if failure
}

/*
 * Insert entry (value1,value2) in fileDesc file
 */
int AM_InsertEntry(
        int fileDesc,
        void *value1,
        void *value2
)
{
    HeaderBlock hBlock;									//
    void * block;									//
    if(BF_ReadBlock(fileDesc, 0, &block) != 0)						//Read the first block and if FAIL return BF error
        return BF_Error();								//
    memcpy(&hBlock, block, sizeof(hBlock));						//
    int currBlockNum = 1;
    if(BF_WriteBlock(fileDesc, 0) != 0)							//write block in the first block of the file
        return BF_Error();
    if(Insert(fileDesc, &hBlock, value1, value2) < 0)					//insert key and second field
    {
//        AM_errno=AME_INSERTENTRYFAILS;
        return AM_errno;
    }								//in case insert function fails

    if(BF_ReadBlock(fileDesc, 0, &block) != 0)
        return BF_Error();
    memcpy(block, &hBlock, sizeof(hBlock));
    if(BF_WriteBlock(fileDesc, 0) != 0)
        return BF_Error();

    return AME_OK;
}


///INSERT
/* Finds the appropriate place to
 * split a node that is too big into two.
 */


/*
 * Cut the length in half
 */
int cutInHalf( int length ) {
    if (length % 2 == 0)								//if length is even
        return length/2;								//return the half of length (eg for length 8, return 8/2 = 4 )
    else										//if length is odd
        return length/2 + 1;								//return length/2 + 1 (eg for length 7, return 7/2 + 1 = 3+1 = 4 )
}

/*
 * Compare and decide if key1 (currKey) < key2 (key)
 */
int isLessThan(char type, int size, void * currKey, void * key)
{
    if(type == 'i')									//if the keys to compare are integers
    {
        int k1 = 0, k2 = 0;
        memcpy(&k1, currKey, size);							//copy currKey in k1
        memcpy(&k2, key, size);								//copy key in k2
        return k1 < k2;									//returns true if k1<k2, else false
    }
    if(type == 'c')									//if the keys to compare are strings
    {
        char  k1[size], k2[size];
        memcpy(&k1, currKey, size);							//string copy currKey in k1
        memcpy(&k2, key, size);								//string copy key in k2
        return strcmp(k1, k2) < 0;							//returns true if k1<k2, else false
    }
    if(type == 'f')
    {
        float  k1,k2;
        memcpy(&k1, currKey, size);							//copy currKey in k1
        memcpy(&k2, key, size);								//copy key in k2
        return k1 < k2;									//returns true if k1<k2, else false
    }
}

/*
 * Compare and decide if key1 (currKey) <= key2 (key)
 */
int isLessThanOrEqual(char type, int size, void * currKey, void * key)
{
    if(type == 'i')									//if the keys to compare are integers
    {
        int k1 = 0, k2 = 0;
        memcpy(&k1, currKey, size);							//copy currKey in k1
        memcpy(&k2, key, size);								//copy key in k2
        return k1 <= k2;								//returns true if k1<=k2, else false
    }
    if(type == 'c')
    {
        char  k1[size], k2[size];
        memcpy(&k1, currKey, size);							//string copy currKey in k1
        memcpy(&k2, key, size);								//string copy key in k2
        return strcmp(k1, k2) <= 0;							//returns true if k1<=k2, else false
    }
    if(type == 'f')
    {
        float  k1,k2;
        memcpy(&k1, currKey, size);							//copy currKey in k1
        memcpy(&k2, key, size);								//copy key in k2
        return k1 <= k2;								//returns true if k1<=k2, else false
    }
}

/*
 * Find the Data Block with given key
 */
int findDataBlock(int fd, HeaderBlock * headerBlock, void * key) {
    int i = 0;

    void * block;
    void * currKey = malloc(headerBlock->keySize);						//allocate memory equal to keysize for currkey
    int currPtr = headerBlock->root;

    if(BF_ReadBlock(fd, headerBlock->root, &block) != 0)					//Retrive data from root
    {
        BF_PrintError("1");									//ReadBlock failed
        return BF_Error();
    }
    IndexBlockInfo iInfo;
    memcpy(&iInfo, block, sizeof(IndexBlockInfo));						//copy data to iInfo from block (root in this case)
    //printf("searching for dataBlock with key : ");       //TODO make function for key print
    //PrintKey(headerBlock->keyType, headerBlock->keySize, key);
    if(BF_WriteBlock(fd, headerBlock->root) != 0)
        return BF_Error();
    int k = 0;
    while (iInfo.isIndexBlock) {								//while iInfo remains an Index Block
        ++k;
        i = 0;
        while (i < iInfo.CurrNumOfKeys) {							//scan all the keys in iInfo until key<currkey
//            currPtr = 0;      //CHECKARW AN XREIAZONTAI
//            memcpy(currKey, &currPtr, sizeof(int));
            memcpy(currKey,
                   (block + getIndexKeyPos(i, headerBlock->keySize)),
                   headerBlock->keySize);
            if(!isLessThan(headerBlock->keyType, headerBlock->keySize, key, currKey))
                i++;
            else
                break;
            memcpy(&currPtr,
                   (block + getIndexPtrPos(i, headerBlock->keySize)),
                   headerBlock->ptrSize);


            //printf("pointer : %d currentKey is :", currPtr);
            //PrintKey(headerBlock->keyType, headerBlock->keySize, currKey);
//            PrintKey(headerBlock->keyType, headerBlock->keySize, key);
//            fflush(stdout);
        }
        //PrintKey(headerBlock->keyType, headerBlock->keySize, currKey);
        //PrintKey(headerBlock->keyType, headerBlock->keySize, key);

        memcpy(&currPtr,
               (block + getIndexPtrPos(i, headerBlock->keySize)),
               headerBlock->ptrSize);
        //printf("pointer : %d\n", currPtr);
        if(currPtr == 0)
            sleep(100);
        if(BF_ReadBlock(fd, currPtr, &block) != 0)				//add iInfo in file
            return BF_Error();
        memcpy(&iInfo, block, sizeof(IndexBlockInfo));
        if(BF_WriteBlock(fd, currPtr) != 0)     //free block
            return BF_Error();
    }
    free(currKey);
    //printf("returning pointer : %d\n", currPtr);
    return currPtr;
}




/*
    Get the internal position of the left ptr in the Index Node (parent) and also
    allocate space and return the parentBlock
 */
int getIndexPos(int fd, int parent, int left, HeaderBlock * headerBlock, IndexBlockInfo * parentInfo, void ** parentBlock) {


    if(BF_ReadBlock(fd, parent, parentBlock) != 0)
    {
        BF_PrintError("getLeftIndex");
        //AM_errno = AME_BF;
        //return AM_errno;
	    return BF_Error();
    }
    memcpy(parentInfo, *parentBlock, sizeof(IndexBlockInfo));

    int leftIndex = 0;
    int currParentPointer;
    memcpy(&currParentPointer,
           *(parentBlock) + getIndexPtrPos(leftIndex, headerBlock->keySize),
           headerBlock->ptrSize);
    while (leftIndex <= parentInfo->CurrNumOfKeys &&
            currParentPointer != left)
    {
        leftIndex++;
        memcpy(&currParentPointer, *(parentBlock) + getIndexPtrPos(leftIndex, headerBlock->keySize), headerBlock->ptrSize);
    }
    //printf("left is %d leftIndex is %d\n",left ,leftIndex);
    return leftIndex;
}




/*
    Insert a new Key and secField into the datablock
 */
int insertIntoDataBlock(int fd, HeaderBlock * headerBlock, DataBlockInfo lInfo, void * dataBlock, int dataBlockPtr, void * key, void * secField )  {
    int i, insertionPoint;


    insertionPoint = 0;
    void * currKey = malloc(headerBlock->keySize);
    memcpy(currKey,
           dataBlock + getDataBlockKeyPos(insertionPoint, headerBlock->keySize, headerBlock->secFieldSize),
           headerBlock->keySize);
    while (insertionPoint < lInfo.CurrNumOfValues &&
            isLessThan(headerBlock->keyType, headerBlock->keySize, currKey, key))
    {
        insertionPoint++;
        memcpy(currKey,
               dataBlock + getDataBlockKeyPos(insertionPoint, headerBlock->keySize, headerBlock->secFieldSize),
               headerBlock->keySize);
    }

    for (i = lInfo.CurrNumOfValues ; i > insertionPoint; i--) {
        memcpy(dataBlock + getDataBlockKeyPos(i, headerBlock->keySize, headerBlock->secFieldSize),
               dataBlock + getDataBlockKeyPos(i - 1, headerBlock->keySize, headerBlock->secFieldSize),
               headerBlock->keySize);
        memcpy(dataBlock + getDataBlockSecFieldPos(i, headerBlock->keySize, headerBlock->secFieldSize),
               dataBlock + getDataBlockSecFieldPos(i - 1, headerBlock->keySize, headerBlock->secFieldSize),
               headerBlock->secFieldSize);
    }
    memcpy(dataBlock + getDataBlockKeyPos(insertionPoint, headerBlock->keySize, headerBlock->secFieldSize),
           key,
           headerBlock->keySize);
    memcpy(dataBlock + getDataBlockSecFieldPos(insertionPoint, headerBlock->keySize, headerBlock->secFieldSize),
           secField,
           headerBlock->secFieldSize);
    lInfo.CurrNumOfValues++;
    memcpy(dataBlock, &lInfo, sizeof(DataBlockInfo));
    if(BF_WriteBlock(fd, dataBlockPtr) != 0)
        return BF_Error();
    //printf("\nInserted into dataBlock : %d\n", dataBlockPtr);
    free(currKey);
    return dataBlockPtr;
}


/*
 * The data block is full so it must be split
 * Allocate space in memory for all the keys and insert the new key into position
 * then split keys in half and distribute them evenly between the two new Data Blocks
 */
int insertIntoDataBlockAfterSplitting(int fd, HeaderBlock * headerBlock, DataBlockInfo lInfo, void * dataBlock, int dataBlockPtr, void * key, void * secField ) {
    void * tempKeys;
    void * tempSecFields;
    int insertionIndex, half, new_key, i, j;

    DataBlockInfo newDataBlockInfo;

    newDataBlockInfo.CurrNumOfValues = 0;
    newDataBlockInfo.isIndexBlock = 0;

    int totalKeyValuePairs = headerBlock->NumOfRecsInDataBlock + 1;
    insertionIndex = 0;
    void * dataBlockKeys = malloc((totalKeyValuePairs)  * headerBlock->keySize);      //save all the keys of the dataBlock
    void * dataBlockSecFields = malloc((totalKeyValuePairs)  * headerBlock->secFieldSize);    //save all sec fields of the dataBlock
    int k;
    for (k = 0; k < lInfo.CurrNumOfValues; ++k) {
        memcpy((dataBlockKeys + k * headerBlock->keySize),
               (dataBlock + getDataBlockKeyPos(k, headerBlock->keySize, headerBlock->secFieldSize))
                ,headerBlock->keySize );
        memcpy((dataBlockSecFields + k * headerBlock->secFieldSize),
               (dataBlock + getDataBlockSecFieldPos(k, headerBlock->keySize, headerBlock->secFieldSize))
                ,headerBlock->secFieldSize);
    }

    while(insertionIndex < headerBlock->NumOfRecsInDataBlock &&
            isLessThan(headerBlock->keyType,
                       headerBlock->keySize,
                       dataBlockKeys + insertionIndex * headerBlock->keySize,
                       key))
        insertionIndex++;

    for (i = totalKeyValuePairs - 2; i >= insertionIndex; i-- )       //make room for key at insertion index
    {
        memcpy(dataBlockKeys + (i + 1) * headerBlock->keySize, dataBlockKeys + (i) * headerBlock->keySize, headerBlock->keySize);
        memcpy(dataBlockSecFields + (i + 1) * headerBlock->secFieldSize, dataBlockSecFields + (i) * headerBlock->secFieldSize, headerBlock->secFieldSize);
    }

    memcpy(dataBlockKeys + insertionIndex * headerBlock->keySize, key, headerBlock->keySize);
    memcpy(dataBlockSecFields + insertionIndex * headerBlock->secFieldSize, secField, headerBlock->secFieldSize);


    half = cutInHalf(totalKeyValuePairs);     //split keys and sec fields in half
    //check if the are Equal Keys to avoid splitting on equal ones
    while(areEqual(headerBlock->keyType,
                   headerBlock->keySize, dataBlock + getDataBlockKeyPos(half, headerBlock->keySize, headerBlock->secFieldSize),
                   dataBlock + getDataBlockKeyPos(half - 1, headerBlock->keySize, headerBlock->secFieldSize))
            && half >= 1)
    {
        half--;
    }
    for(i = 0; i < half; i++)
    {
        memcpy(dataBlock + getDataBlockKeyPos(i, headerBlock->keySize, headerBlock->secFieldSize),
               dataBlockKeys + i * headerBlock->keySize,
               headerBlock->keySize);
        memcpy(dataBlock + getDataBlockSecFieldPos(i, headerBlock->keySize, headerBlock->secFieldSize),
               dataBlockSecFields + i * headerBlock->secFieldSize,
               headerBlock->secFieldSize);
    }

    lInfo.CurrNumOfValues = half;

    newDataBlockInfo.nextBlock = lInfo.nextBlock;
    newDataBlockInfo.parentBlock = lInfo.parentBlock;
    newDataBlockInfo.isIndexBlock = 0;
    void * newKey = malloc(headerBlock->keySize);


    void * newDataBlock;
    if(BF_AllocateBlock(fd) != 0)
        return BF_Error();

    int newDataBlockPtr = BF_GetBlockCounter(fd) - 1;
    lInfo.nextBlock = newDataBlockPtr;
    memcpy(dataBlock, &lInfo, sizeof(DataBlockInfo));
    if(BF_WriteBlock(fd, dataBlockPtr) != 0)
        return BF_Error();

    if(BF_ReadBlock(fd, newDataBlockPtr, &newDataBlock) != 0)
        return BF_Error();
    for (i = 0, j = lInfo.CurrNumOfValues; j < totalKeyValuePairs; i++, j++) {
        memcpy(newDataBlock + getDataBlockKeyPos(i, headerBlock->keySize, headerBlock->secFieldSize),
               dataBlockKeys + j * headerBlock->keySize,
               headerBlock->keySize);
        memcpy(newDataBlock + getDataBlockSecFieldPos(i, headerBlock->keySize, headerBlock->secFieldSize),
               dataBlockSecFields + j * headerBlock->secFieldSize,
               headerBlock->secFieldSize);
    }
    newDataBlockInfo.CurrNumOfValues = totalKeyValuePairs - lInfo.CurrNumOfValues;
    memcpy(newDataBlock, &newDataBlockInfo, sizeof(DataBlockInfo));
    memcpy(newKey, dataBlockKeys + half * headerBlock->keySize, headerBlock->keySize);       //middle key is inserted into parent
    if(BF_WriteBlock(fd, newDataBlockPtr) != 0)
        return BF_Error();


    free(dataBlockKeys);
    free(dataBlockSecFields);

    return insertIntoParent(fd, lInfo.parentBlock, dataBlockPtr, newKey, newDataBlockPtr, headerBlock);
}



/*
 * Insert key and right pointer into left index position by moving all the keys to the right
 * One position.
 */
int insertIntoIndexNode(int fd, int node, int leftIndex,
                   void * key, int right,
                   IndexBlockInfo * nBlockInfo,
                   void * nBlock, HeaderBlock * headerBlock) {
    int i;  //EDW KSANAKOITAW LOGIC

    for (i = nBlockInfo->CurrNumOfKeys; i > leftIndex; i--) {
        memcpy(nBlock + getIndexPtrPos(i + 1, headerBlock->keySize),
               nBlock + getIndexPtrPos(i, headerBlock->keySize),
               headerBlock->ptrSize);
        memcpy(nBlock + getIndexKeyPos(i, headerBlock->keySize),
               nBlock + getIndexKeyPos(i - 1, headerBlock->keySize),
               headerBlock->keySize);
    }
    memcpy(nBlock + getIndexPtrPos(leftIndex + 1, headerBlock->keySize),
           &right,
           sizeof(int));
    memcpy(nBlock + getIndexKeyPos(leftIndex, headerBlock->keySize),
           key,
           headerBlock->keySize);
    nBlockInfo->CurrNumOfKeys++;
    memcpy(nBlock, nBlockInfo, sizeof(IndexBlockInfo));

//    int test;
//    for (i = 0; i < nBlockInfo->CurrNumOfKeys; ++i) {
//        memcpy(&test, nBlock + getIndexPtrPos(i, headerBlock->keySize), headerBlock->ptrSize);
//        printf("**%d**",test);
//
//        memcpy(&test, nBlock + getIndexKeyPos(i, headerBlock->keySize), headerBlock->keySize);
//        printf("**%d**\n",test);
//    }
//    memcpy(&test, nBlock + getIndexPtrPos(i, headerBlock->keySize), headerBlock->ptrSize);
//    printf("**%d**\n\n\n\n\n",test);
//    fflush(stdout);

    if(BF_WriteBlock(fd, node) != 0)
    {
        //AM_errno = AME_BF;
        //return AM_errno;
	    return BF_Error();
    }
    //printf("Inserted into node : %d pointer : %d with Key: ", node, right);
    //PrintKey(headerBlock->keyType, headerBlock->keySize, key);
    return headerBlock->root;
}

/*
    Index Node is full so it must be split follow the same strategy as you did with the Data Block
    the only difference is that
    The Middle key is now moved up and does not stay in any of the Blocks that have been split
 */
int insertIntoIndexNodeAfterSplitting(int fd, int oldIndexNode, int leftIndex, void * key,
                                 int right, IndexBlockInfo * oldNodeInfo,
                                 void * oldNodeBlock, HeaderBlock * headerBlock)
{


//    printf("Insert Into Node After Splitting \n");
    void * kPrime = malloc(headerBlock->keySize);
    void * nodeKeys;
    int * nodePointers;
    int insertionIndex, split, new_key, i, j;
    int test;
    int totalNumOfKeys = headerBlock->NumOfKeysInIndexBlock + 1;
    nodeKeys = malloc(totalNumOfKeys * headerBlock->keySize);
    if (nodeKeys == NULL) {
        perror("Node keys array.");
        //return AME_BF;
	    return BF_Error();
    }

    nodePointers = (int *) malloc((totalNumOfKeys + 1) * sizeof(int));
    if (nodePointers == NULL) {
        perror("Node pointers array.");
        //return AME_BF;
	    return BF_Error();
    }

    for (i = 0, j = 0; i < oldNodeInfo->CurrNumOfKeys + 1; i++, j++) {
        if (j == leftIndex + 1) j++;
        memcpy(&nodePointers[j],
               oldNodeBlock + getIndexPtrPos(i, headerBlock->keySize),
               headerBlock->ptrSize);
        //printf("pointer : %d\n", nodePointers[j]);
    }

    for (i = 0, j = 0; i < oldNodeInfo->CurrNumOfKeys; i++, j++) {
        if (j == leftIndex) j++;
        memcpy(nodeKeys + j * headerBlock->keySize,
               oldNodeBlock + getIndexKeyPos(i, headerBlock->keySize),
               headerBlock->keySize);
    }

    memcpy(&nodePointers[leftIndex + 1], &right, headerBlock->ptrSize);
    memcpy(nodeKeys + leftIndex * headerBlock->keySize, key, headerBlock->keySize);

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */

    split = cutInHalf(totalNumOfKeys);
    oldNodeInfo->CurrNumOfKeys = 0;
    //printf("\nWriting to new left block\n");
    for (i = 0; i < split - 1; i++) {
        memcpy(oldNodeBlock + getIndexPtrPos(i, headerBlock->keySize),
               &nodePointers[i],
               headerBlock->ptrSize);
        memcpy(oldNodeBlock + getIndexKeyPos(i, headerBlock->keySize),
               nodeKeys + i * headerBlock->keySize,
               headerBlock->keySize);
        oldNodeInfo->CurrNumOfKeys++;
        //printf("pointer : %d", nodePointers[i]);
        //printf(" Key : ");
        //PrintKey(headerBlock->keyType, headerBlock->keySize, nodeKeys + i * headerBlock->keySize);
    }
    memcpy(oldNodeBlock + getIndexPtrPos(i, headerBlock->keySize),
           &nodePointers[i],
           headerBlock->ptrSize);

    memcpy(kPrime, nodeKeys + (split - 1) * headerBlock->keySize, headerBlock->keySize);
    //PrintKey(headerBlock->keyType, headerBlock->keySize, kPrime);
    memcpy(oldNodeBlock, oldNodeInfo, sizeof(IndexBlockInfo));
    if(BF_WriteBlock(fd, oldIndexNode) != 0)
        return BF_Error();


    if(BF_AllocateBlock(fd) != 0)
        return BF_Error();
    int newNode = BF_GetBlockCounter(fd) - 1;
    void * newIndexNodeBlock;
    if(BF_ReadBlock(fd, newNode, &newIndexNodeBlock) != 0)
        return BF_Error();
    IndexBlockInfo newIndexNodeInfo;
    newIndexNodeInfo.isIndexBlock = 1;
    newIndexNodeInfo.parentBlock = oldNodeInfo->parentBlock;
    newIndexNodeInfo.CurrNumOfKeys = 0;
    int child = 0;
    void * childBlock;

    //printf("\nWriting to new right block\n");
    for (i = split, j = 0; i < totalNumOfKeys; i++, j++) {
        memcpy(newIndexNodeBlock + getIndexPtrPos(j, headerBlock->keySize),
               &nodePointers[i],
               headerBlock->ptrSize);
        memcpy(newIndexNodeBlock + getIndexKeyPos(j, headerBlock->keySize),
               nodeKeys + i * headerBlock->keySize,
               headerBlock->keySize);
        newIndexNodeInfo.CurrNumOfKeys++;
        //printf("pointer : %d", nodePointers[i]);
        //printf(" Key : ");
        //PrintKey(headerBlock->keyType, headerBlock->keySize, nodeKeys + i * headerBlock->keySize);
    }
    memcpy(newIndexNodeBlock,
           &newIndexNodeInfo,
           sizeof(IndexBlockInfo));
    memcpy(newIndexNodeBlock + getIndexPtrPos(j, headerBlock->keySize),
           &nodePointers[i],
           headerBlock->ptrSize);                           //pointers are one more than the num of keys


    if(BF_WriteBlock(fd, newNode) != 0)
        return BF_Error();

    //ISSUE here if you read too many blocks old blocks will start to get freed and that happened to the newNode blocks
    for (i = split; i < totalNumOfKeys + 1; i++) {
        memcpy(&child,&nodePointers[i], sizeof(int));
        if(BF_ReadBlock(fd, child, &childBlock) != 0)
            return BF_Error();
        memcpy(childBlock + PARENT_OFFSET * sizeof(int), &newNode, sizeof(int));
        if(BF_WriteBlock(fd, child) != 0) {
            return BF_Error();
        }
    }


    free(nodePointers);
    free(nodeKeys);
    //printf("Node : %d split into : %d and : %d and moving key : ", oldIndexNode, oldIndexNode, newNode);
    //PrintKey(headerBlock->keyType, headerBlock->keySize, kPrime);
    //After you are done insert the middle key into the parent
    return insertIntoParent(fd, oldNodeInfo->parentBlock, oldIndexNode, kPrime, newNode, headerBlock);
}


/*
 * Inserts a new key into the parent of the split block (the splitted block is either Index or data block).
 * Returns a positive number
 */
int insertIntoParent(int fd, int parent, int left, void * key, int right, HeaderBlock * headerBlock) {

    int leftIndex;
    //printf("Insert into parent\n");
    /* Case: new root. */
    if (parent == -1)
        return insertIntoNewRoot(fd, left, key, right, headerBlock);

    /* Case: dataBlock or node.
     */

    /* find the index of the left pointer in the parent
     */
    void * parentBlock;
    IndexBlockInfo parentInfo;
    leftIndex = getIndexPos(fd, parent, left, headerBlock, &parentInfo, &parentBlock);


    /*the new key fits into the Index node.
     */
    if (parentInfo.CurrNumOfKeys < headerBlock->NumOfKeysInIndexBlock)
        return insertIntoIndexNode(fd, parent, leftIndex, key, right, &parentInfo, parentBlock, headerBlock);

    /* Split the Index Node in two and insert the key into the parent
     */

    return insertIntoIndexNodeAfterSplitting(fd, parent, leftIndex, key, right, &parentInfo, parentBlock, headerBlock);
}

/*
    Create a new Root after a key was inserted the root must be split in two and a new root must be
    Created above the two subtrees
 */
int insertIntoNewRoot(int fd, int left, void * key, int right, HeaderBlock * headerBlock) {
    IndexBlockInfo leftInfo, rightInfo;
    IndexBlockInfo newRootInfo;
    newRootInfo.CurrNumOfKeys = 1;
    newRootInfo.parentBlock = -1;
    newRootInfo.isIndexBlock = 1;
    void * rootBlock;
    if(BF_AllocateBlock(fd) != 0)
        return BF_Error();
    int newRoot = BF_GetBlockCounter(fd) - 1;

    if(BF_ReadBlock(fd, newRoot, &rootBlock) != 0)
        return BF_Error();
    memcpy(rootBlock, &newRootInfo, sizeof(IndexBlockInfo));
    memcpy(rootBlock + sizeof(IndexBlockInfo) + headerBlock->ptrSize, key, headerBlock->keySize);
    memcpy(rootBlock + sizeof(IndexBlockInfo), &left, headerBlock->ptrSize);
    memcpy(rootBlock + sizeof(IndexBlockInfo) + headerBlock->ptrSize + headerBlock->keySize, &right, headerBlock->ptrSize);
    if(BF_WriteBlock(fd, newRoot) != 0)
        return BF_Error();

    void * block;
    if(BF_ReadBlock(fd, left, &block) != 0)
    {
//        BF_PrintError("insert new root");
	    return BF_Error();
    }
    memcpy(&leftInfo, block, sizeof(IndexBlockInfo));
    leftInfo.parentBlock = newRoot;
    memcpy(block, &leftInfo, sizeof(IndexBlockInfo));
    if(BF_WriteBlock(fd, left) != 0)
    {
//        BF_PrintError("insert new root");
	    return BF_Error();
    }

    if(BF_ReadBlock(fd, right, &block) != 0)
        return BF_Error();
    memcpy(&rightInfo, block, sizeof(IndexBlockInfo));
    rightInfo.parentBlock = newRoot;
    memcpy(block, &rightInfo, sizeof(IndexBlockInfo));
    if(BF_WriteBlock(fd, right) != 0)
        return BF_Error();



    headerBlock->root = newRoot;
    //printf("New Root is %d", headerBlock->root);
    return newRoot;
}

///* First insertion:
// * start a new tree.
// */
int initTree(int fd, HeaderBlock * headerBlock, void * keyField, void * secField) {
    void * block;
    //initialize root block
    if(BF_AllocateBlock(fd) != 0)
        return BF_Error();
    headerBlock->root = BF_GetBlockCounter(fd) - 1;

    if(BF_ReadBlock(fd, headerBlock->root, &block) != 0)
        return BF_Error();
    DataBlockInfo rootInfo;
    rootInfo.parentBlock = -1;
    rootInfo.nextBlock = -1;
    rootInfo.CurrNumOfValues = 1;
    rootInfo.isIndexBlock = 0;
    memcpy(block, &rootInfo, sizeof(DataBlockInfo));
    memcpy((block + getDataBlockKeyPos(0, headerBlock->keySize, headerBlock->secFieldSize)),
           keyField, (headerBlock->keySize));
    memcpy((block + getDataBlockSecFieldPos(0, headerBlock->keySize, headerBlock->secFieldSize)),
           secField, (headerBlock->secFieldSize));
    if(BF_WriteBlock(fd, headerBlock->root) != 0)
        return BF_Error();

//    if(BF_ReadBlock(fd, rightPtr, &block) != 0)
//        return BF_Error();
//
//    DataBlockInfo dataBlockInfo;
//    dataBlockInfo.parentBlock = headerBlock->root;
//    dataBlockInfo.nextBlock = -1;
//    dataBlockInfo.isIndexBlock = 0;
//    dataBlockInfo.CurrNumOfValues = 1;
//    memcpy(block, &dataBlockInfo, sizeof(DataBlockInfo));
//    memcpy((block + sizeof(DataBlockInfo)), keyField, (headerBlock->keySize));
//    memcpy((block + sizeof(DataBlockInfo) + headerBlock->keySize), secField, (headerBlock->secFieldSize));
//    if(BF_WriteBlock(fd, rightPtr) != 0)
//        return BF_Error();
//
//    //write in block
//    if(BF_ReadBlock(fd, leftPtr, &block) != 0)
//        return BF_Error();
//    dataBlockInfo.parentBlock = headerBlock->root;
//    dataBlockInfo.nextBlock = rightPtr;
//    dataBlockInfo.isIndexBlock = 0;
//    dataBlockInfo.CurrNumOfValues = 0;
//    memcpy(block, &dataBlockInfo, sizeof(DataBlockInfo));
//    if(BF_WriteBlock(fd, leftPtr) != 0)
//        return BF_Error();

    return headerBlock->root;
}

/*
 * Insert block (key,second field) in tree
 */
int Insert(int fd, HeaderBlock * headerBlock, void * keyField, void * secField)
{
    /* Case: the tree does not exist yet.
     * Start a new tree.
     */
    void * block;
    if (headerBlock->isEmpty == 1)            //if tree is empty, initialize it
    {
        headerBlock->isEmpty = 0 ;
        if(BF_ReadBlock(fd, 0, &block) != 0)
            return BF_Error();
        memcpy(block, headerBlock, sizeof(HeaderBlock));
        if(BF_WriteBlock(fd, 0) != 0)
            return BF_Error();
        return initTree(fd, headerBlock, keyField, secField);
    }
    /* Case: the tree already exists.
     * (Rest of function body.)
     */
    int dataBlockPtr = findDataBlock(fd, headerBlock, keyField);
    if(dataBlockPtr < 0)
        return dataBlockPtr;
    /* Case: dataBlock has room for key and pointer.
     */
    if(BF_ReadBlock(fd, dataBlockPtr, &block) != 0)
        return BF_Error();
    DataBlockInfo dataBlockInfo;
    memcpy(&dataBlockInfo, block, sizeof(DataBlockInfo));
    if (dataBlockInfo.CurrNumOfValues < headerBlock->NumOfRecsInDataBlock) {
        dataBlockPtr = insertIntoDataBlock(fd, headerBlock, dataBlockInfo, block, dataBlockPtr, keyField, secField);
        if(dataBlockPtr < 0)
            return dataBlockPtr;
        return headerBlock->root;
    }
    /* Case:  dataBlock must be split.
     */
    return insertIntoDataBlockAfterSplitting(fd, headerBlock, dataBlockInfo, block, dataBlockPtr, keyField, secField);
}

//open Index Scan

/*
 * Print the Record (key,second field)
 */
void printRecord(HeaderBlock headerBlock, void * key, void * secField)
{
    printf("***Record***\n");
    printf("Key :");
    PrintKey(headerBlock.keyType, headerBlock.keySize, key);					//Print key of any type
    printf("SecField :");
    PrintKey(headerBlock.secFieldType, headerBlock.secFieldSize, secField);			//Print second field of any type
    printf("*******\n");

}

/*
 * Find the Left Most Block in filedesc
 */
int findLeftMostBlock(int fileDesc, HeaderBlock * headerBlock)
{
    int i = 0;

    void * block;
    void * currKey = malloc(headerBlock->keySize);
    int currPtr;
    //read the root and copy it to iInfo
    if(BF_ReadBlock(fileDesc, headerBlock->root, &block) != 0)
        return BF_Error();
    IndexBlockInfo iInfo;
    memcpy(&iInfo, block, sizeof(IndexBlockInfo));
    //while iInfo is Index Block
    while (iInfo.isIndexBlock) {
        memcpy(&currPtr,
               (block + getIndexPtrPos(0, headerBlock->keySize)),
               headerBlock->ptrSize);
        //printf("pointer : %d\n", currPtr);
        if(currPtr == 0)
            sleep(100);
        if(BF_ReadBlock(fileDesc, currPtr, &block) != 0)
            return BF_Error();
        memcpy(&iInfo, block, sizeof(IndexBlockInfo));
        if(BF_WriteBlock(fileDesc, currPtr) != 0)     //free block
            return BF_Error();
    }
    free(currKey);
    //printf("returning pointer : %d\n", currPtr);
    return currPtr;
}

/*
 * Scan file fileDesc to find Data Blocks with key (operation op) value
 */
int AM_OpenIndexScan(
        int fileDesc,
        int op,
        void *value
)
{
    int i;
    if(currPosOfScansArray == MAXSCANS)						//Maximum simltaneous scan cap reached, cannot continue
    {
        AM_errno=AME_MAXSCANSFULL;
	    return AME_MAXSCANSFULL;
    }
    globalFileScansArray[currPosOfScansArray].fileDesc = fileDesc;		//
    globalFileScansArray[currPosOfScansArray].op = op;				//Insert the data in the list of simultaneous scans
    globalFileScansArray[currPosOfScansArray].currEntryPos = 0;			//
    globalFileScansArray[currPosOfScansArray].key = value;			//
    void * block;
    if(BF_ReadBlock(fileDesc, 0, &block) != 0)					//Read the first block
        return BF_Error();
    HeaderBlock headerBlock;
    memcpy(&headerBlock, block, sizeof(HeaderBlock));				//copy the read block in headerBlock
    if(op == EQUAL || op == GREATER_THAN || op == GREATER_THAN_OR_EQUAL)
        globalFileScansArray[currPosOfScansArray].dataBlock = findDataBlock(fileDesc, &headerBlock, value);//Data block is the found
    else
        globalFileScansArray[currPosOfScansArray].dataBlock = findLeftMostBlock(fileDesc, &headerBlock);//Data block is the left most
    currPosOfScansArray++;							//number of scans in maxscans list + 1
    return currPosOfScansArray - 1;						//return the number of position in maxscans list
}

/*
 * Increase the current Entry Position by 1 or set current E.P. to zero on the ist of file scans 
 */
int addCurrEntryPos(int scanDesc, DataBlockInfo * dataBlockInfo, void ** dataBlock)
{
    if(globalFileScansArray[scanDesc].currEntryPos == dataBlockInfo->CurrNumOfValues - 1)
    {
//        printf("****CHANGING BLOCK****\n");
        globalFileScansArray[scanDesc].dataBlock = dataBlockInfo->nextBlock;
        if(globalFileScansArray[scanDesc].dataBlock == -1)
        {
            AM_errno = AME_EOF;
            return 0;
        }
        if(BF_ReadBlock(globalFileScansArray[scanDesc].fileDesc, globalFileScansArray[scanDesc].dataBlock, dataBlock) != 0)
        {
            BF_Error();
            return 0;
        }
        memcpy(dataBlockInfo, *dataBlock, sizeof(DataBlockInfo));
        globalFileScansArray[scanDesc].currEntryPos = 0;
    }
    else
        globalFileScansArray[scanDesc].currEntryPos++;
    return 1;
}

/*
 * Find the next entry that satisfies the wanted operation. Returns NULL when failed or a pointer at second field when successful
 */
void *AM_FindNextEntry(
        int scanDesc
)
{
    if( scanDesc < 0 || scanDesc >= MAXSCANS || scanDesc > currPosOfScansArray)				//Invalid scanDesc given, return error
    {
        AM_errno = AME_WRONGSCANDESC;
        return NULL;
    }

    if(globalFileScansArray[scanDesc].dataBlock == -1)							//No Datablock on the list
    {
        AM_errno = AME_EOF;
        return NULL;
    }
    int currDataBlock = globalFileScansArray[scanDesc].dataBlock;
    void * block;
    int fileDesc = globalFileScansArray[scanDesc].fileDesc;
    HeaderBlock headerBlock;
    if(BF_ReadBlock(fileDesc, 0, &block) != 0) {
        AM_errno = AME_ERRORINBF;
        return NULL;
    }
    memcpy(&headerBlock, block, sizeof(HeaderBlock));

    if(currDataBlock > BF_GetBlockCounter(fileDesc) - 1 || currDataBlock < 0) {
        AM_errno = AME_WRONGDATABLOCK;
        return NULL;
    }
    void * dataBlock;
    DataBlockInfo dataBlockInfo;
    void * currkey = malloc(headerBlock.keySize);
    void * secField = malloc(headerBlock.secFieldSize);
    if(BF_ReadBlock(fileDesc, globalFileScansArray[scanDesc].dataBlock, &dataBlock) != 0)		//If Reading Block fails
    {
        AM_errno = AME_ERRORINBF;
        return NULL;
    }
    switch (globalFileScansArray[scanDesc].op)								//For the operation type wanted
    {
        case EQUAL :      //op EQUAL

            memcpy(&dataBlockInfo, dataBlock, sizeof(DataBlockInfo));
            while (globalFileScansArray[scanDesc].currEntryPos < dataBlockInfo.CurrNumOfValues) {
                memcpy(currkey,
                       dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                       headerBlock.keySize);
                if(areEqual(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key))
                    break;
                if(!isLessThan(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key))       //if currKey >(=) than value end
                {
                    AM_errno = AME_EOF;
                    return NULL;
                }
                if(!addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock))
                    return NULL;

            }


            memcpy(secField,
                   dataBlock + getDataBlockSecFieldPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.secFieldSize);
            //printRecord(headerBlock, currkey, secField);
            addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock);
            return secField;

        case NOT_EQUAL :      //op is NOT EQUAL

            memcpy(&dataBlockInfo, dataBlock, sizeof(DataBlockInfo));
            memcpy(currkey,
                   dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.keySize);
            while(areEqual(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key) &&
                    globalFileScansArray[scanDesc].currEntryPos < dataBlockInfo.CurrNumOfValues)
            {
                if(!addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock))
                    return NULL;

                memcpy(currkey,
                       dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                       headerBlock.keySize);
            }

            memcpy(secField,
                   dataBlock + getDataBlockSecFieldPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.secFieldSize);

            addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock);
            //printRecord(headerBlock, currkey, secField);
            return secField;

        case LESS_THAN :      //op is LESS THAN

            memcpy(&dataBlockInfo, dataBlock, sizeof(DataBlockInfo));
            memcpy(currkey,
                   dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.keySize);
            if(!isLessThan(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key))       //if currKey >(=) than value end
            {
                AM_errno = AME_EOF;
                return NULL;
            }


            memcpy(secField,
                   dataBlock + getDataBlockSecFieldPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.secFieldSize);
            addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock);
            //printRecord(headerBlock, currkey, secField);
            return secField;

        case GREATER_THAN :      //op is GREATER THAN

            memcpy(&dataBlockInfo, dataBlock, sizeof(DataBlockInfo));
            memcpy(currkey,
                   dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.keySize);

            while(globalFileScansArray[scanDesc].currEntryPos < dataBlockInfo.CurrNumOfValues &&
                  isLessThanOrEqual(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key))
            {
                if(!addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock))
                    return NULL;

                memcpy(currkey,
                       dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                       headerBlock.keySize);
            }

            memcpy(secField,
                   dataBlock + getDataBlockSecFieldPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.secFieldSize);
            addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock);
            //printRecord(headerBlock, currkey, secField);
            return secField;

        case LESS_THAN_OR_EQUAL :      //op is LESS THAN OR EQUAL

            memcpy(&dataBlockInfo, dataBlock, sizeof(DataBlockInfo));
            memcpy(currkey,
                   dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.keySize);
            if(!isLessThanOrEqual(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key))       //if currKey >(=) than value end
            {
                AM_errno = AME_EOF;
                return NULL;
            }

            memcpy(secField,
                   dataBlock + getDataBlockSecFieldPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.secFieldSize);

            addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock);


            //printRecord(headerBlock, currkey, secField);
            return secField;

        case GREATER_THAN_OR_EQUAL :      //op is GREATER THAN OR EQUAL

            memcpy(&dataBlockInfo, dataBlock, sizeof(DataBlockInfo));
            memcpy(currkey,
                   dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.keySize);
            while(globalFileScansArray[scanDesc].currEntryPos < dataBlockInfo.CurrNumOfValues &&
                    isLessThan(headerBlock.keyType, headerBlock.keySize, currkey, globalFileScansArray[scanDesc].key))
            {
                if(!addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock))
                    return NULL;

                memcpy(currkey,
                       dataBlock + getDataBlockKeyPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                       headerBlock.keySize);
            }
            memcpy(secField,
                   dataBlock + getDataBlockSecFieldPos(globalFileScansArray[scanDesc].currEntryPos, headerBlock.keySize, headerBlock.secFieldSize),
                   headerBlock.secFieldSize);
            //printRecord(headerBlock, currkey, secField);
            addCurrEntryPos(scanDesc, &dataBlockInfo, &dataBlock);
            return secField;
    }
    AM_errno = AME_WRONGOP;
    return NULL;										//Everyhing failed(invalid op, none of the cases)
}

/*
 * Remove a scan from the file scans list
 */
int AM_CloseIndexScan(
        int scanDesc
)
{
    int j;
    //TODO :ADD errors
    if(scanDesc < 0 || scanDesc >= currPosOfArray)		  //Invalid scansDesc given
        return AME_WRONGSCANDESC;
    for (j = scanDesc; j < currPosOfScansArray - 1; ++j) {        //move every scan a position backwards
        globalFileScansArray[j] = globalFileScansArray[j + 1];
    }
    currPosOfScansArray--;                                       //reduce the number of simultaneous file scans by 1
    return AME_OK;                                		 //return AME_OK
}

/*
 * Print the error message
 */
void AM_PrintError(
        char *errString
)
{
    printf("%s\n", errString);
    if(AM_errno == AME_BF || AM_errno == AME_ERRORINBF)
    {
        BF_PrintError(errString);
    }
    else if(AM_errno==AME_FILEISOPEN) printf("File is arleady open\n\n");
    else if(AM_errno==AME_MAXFILESOPEN) printf("Cannot open more files. Maximum capacity reached\n\n");
    else if(AM_errno==AME_CREATEINDEXFAILS) printf("Failed to create a File. Improper attributes\n\n");
    else if(AM_errno==AME_DESTROYINDEXFAILS) printf("Failed to destroy a File\n\n");
    else if(AM_errno==AME_CLOSEINDEXFAILS) printf("Cannot close File\n\n");
    else if(AM_errno==AME_INSERTENTRYFAILS) printf("Cannot insert entry\n\n");
    else if(AM_errno==AME_MAXSCANSFULL) printf("Maximum scans capacity reached\n\n");
    else if(AM_errno==AME_NODATABLOCK) printf("No datablock found\n\n");
    else if(AM_errno==AME_WRONGSCANDESC) printf("Invalid scan Description given\n\n");
    else if(AM_errno==AME_WRONGDATABLOCK) printf("Wrong Data Block\n\n");
    else if(AM_errno==AME_WRONGOP) printf("Wrong Operation given\n\n");
    else if(AM_errno==AME_FILE_EXISTS) printf("File already exists\n\n");
    else if(AM_errno==AME_FILESCANNING) printf("File is open in scans list and cannot be closed\n\n");

}
