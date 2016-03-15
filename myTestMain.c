

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
        printf("record id : %d\n", recid);
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

    strcpy(empName, "EMP-NAME");
    strcpy(empAge, "EMPAGE");
    strcpy(empSal, "EMP-SAL");
    strcpy(dnameAge, "DNAME-AGE");
    strcpy(fltname, "EMP-FAULT");



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
    recordid = 0;
    srand(time(NULL));
    eage = 8;
    insertEntries(eAentry, eage, ++recordid);
    eage = 22;
    insertEntries(eAentry, eage, ++recordid);
    int i;
    for (i = 1; i < 100000; ++i) {
        eage = abs(rand() % 100000);
        insertEntries(eAentry, eage, ++recordid);
//        printf("record id : %d\n", recordid);
    }
    eage = 40;
    insertEntries(eAentry, eage, ++recordid);
    eage = 5000;
    insertEntries(eAentry, eage, ++recordid);
    eage = 70538;
    insertEntries(eAentry, eage, ++recordid);
    eage = 2594;
    insertEntries(eAentry, eage, ++recordid);

    int scan = AM_OpenIndexScan(eAentry, EQUAL, &eage);
    while(AM_FindNextEntry(scan) != NULL);
    AM_CloseIndexScan(scan);

    eage = 40;
    scan = AM_OpenIndexScan(eAentry, EQUAL, &eage);
    while(AM_FindNextEntry(scan) != NULL);
    AM_CloseIndexScan(scan);

    eage = 5000;
    scan = AM_OpenIndexScan(eAentry, EQUAL, &eage);
    while(AM_FindNextEntry(scan) != NULL);
    AM_CloseIndexScan(scan);

    eage = 70538;
    scan = AM_OpenIndexScan(eAentry, EQUAL, &eage);
    while(AM_FindNextEntry(scan) != NULL);
    AM_CloseIndexScan(scan);

//    for (i = 1; i < 5; ++i) {
//        eage = abs(rand() % 10000);
//        scan = AM_OpenIndexScan(eAentry, EQUAL, &eage);
//        while(AM_FindNextEntry(scan) != NULL);
//        AM_CloseIndexScan(scan);
//    }
    AM_CloseIndex(eAentry);
    AM_DestroyIndex(empAge);
    printf("record id : %d\n", recordid);
    return 0;
}
