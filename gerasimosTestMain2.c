
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defn.h"
#include "AM.h"

char empName[40];
char empAge[40];
char empSal[40];
char dnameAge[40];

void insertEntries(int fd, int age, int recid) {
    char errStr[140];

    strcpy(errStr, "Error in AM_InsertEntry");

    if (AM_InsertEntry(fd, (void *) &age, &recid) != AME_OK) {
        sprintf(errStr, "Error in AM_InsertEntry called on %s \n", empSal);
        AM_PrintError(errStr);
        printf("record id : %d\n", age);
        AM_CloseIndex(fd);
        AM_DestroyIndex("TestDb2");
        exit(0);
    }
}

int main()
{
    int scan1;

    int eNentry;
    int eAentry;
    int eSentry;
    int dAentry;

    int eage;
    float esal;
    char edname[40];
    char ename[40];
    char fltname[40];


    int recordid = 1;

    char errStr[200];

    int* ivalue = NULL;
    char* cvalue = NULL;


    AM_Init();

    strcpy(empAge, "TestDb2");




    if (AM_CreateIndex(empAge, INTEGER, sizeof(int), INTEGER, sizeof(int)) != AME_OK) {
        sprintf(errStr, "Error in AM_CreateIndex called on %s \n", empAge);
        AM_PrintError(errStr);
        return 1;
    }

    if ((eAentry = AM_OpenIndex(empAge)) < 0) {
        sprintf(errStr, "Error in AM_OpenIndex called on %s \n", empAge);
        AM_PrintError(errStr);
        return 1;
    }

    srand(time(NULL));
    int i;
    recordid = 0;
    for (i = 1; i <= 100000; ++i) {
        eage = abs(rand() % 100000);
        insertEntries(eAentry, ++recordid, eage);
//        printf("record id : %d\n", recordid);
    }

    eage = 50000;
    int scan = AM_OpenIndexScan(eAentry, GREATER_THAN, &eage);
    i = 0;
    while(AM_FindNextEntry(scan) != NULL)
        i++;
    printf("***%d***\n", i);
    AM_CloseIndexScan(scan);

    eage = 50000;
    scan = AM_OpenIndexScan(eAentry, LESS_THAN, &eage);
    i = 0;
    while(AM_FindNextEntry(scan) != NULL)
        i++;
    printf("***%d***\n", i);
    AM_CloseIndexScan(scan);

    eage = 50000;
    scan = AM_OpenIndexScan(eAentry, LESS_THAN_OR_EQUAL, &eage);
    i = 0;
    while(AM_FindNextEntry(scan) != NULL)
        i++;
    printf("***%d***\n", i);
    AM_CloseIndexScan(scan);

    eage = 50000;
    scan = AM_OpenIndexScan(eAentry, GREATER_THAN_OR_EQUAL, &eage);
    i = 0;
    while(AM_FindNextEntry(scan) != NULL)
        i++;
    printf("***%d***\n", i);
    AM_CloseIndexScan(scan);

//    for (i = 1; i < 5; ++i) {
//        eage = abs(rand() % 10000);
//        scan = AM_OpenIndexScan(eAentry, EQUAL, &eage);
//        while(AM_FindNextEntry(scan) != NULL);
//        AM_CloseIndexScan(scan);
//    }
    printf("record id : %d\n", recordid);
    AM_CloseIndex(eAentry);
    AM_DestroyIndex(empAge);
    return 0;
}

