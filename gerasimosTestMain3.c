#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "defn.h"
#include "AM.h"

char empName[40];
char empAge[40];
char empSal[40];
char dnameAge[40];

void insertEntries(int fd, char * name, int recid) {
    char errStr[140];

    strcpy(errStr, "Error in AM_InsertEntry");

    if (AM_InsertEntry(fd, name, &recid) != AME_OK) {
        sprintf(errStr, "Error in AM_InsertEntry called on %s \n", empSal);
        AM_PrintError(errStr);
        printf("record id : %d\n", recid);
        AM_CloseIndex(fd);
        AM_DestroyIndex("TestDb3");
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

    empName[9] = '\0';
    char fltname[40];


    int recordid = 1;

    char errStr[200];

    int* ivalue = NULL;
    char* cvalue = NULL;


    AM_Init();

    strcpy(empAge, "TestDb3");




    if (AM_CreateIndex(empAge, STRING, sizeof(empName) - 1, INTEGER, sizeof(int)) != AME_OK) {
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
    int i, j;
    recordid = 0;
    int count = 0;
    for (i = 1; i <= 100000; ++i) {
        for (j = 0; j < 9; ++j) {
            empName[j] = (char) abs(rand()%(122-97) + 97);
        }
        if(empName[0] == 'a')
            count++;
        insertEntries(eAentry, empName, ++recordid);
//        printf("record id : %d\n", recordid);
    }
    strcpy(empName, "bla");
    insertEntries(eAentry, empName, ++recordid);

    printf("names starting with a : %d\n", count);
    empName[0] = 'b';
    empName[1] = '\0';
    int scan = AM_OpenIndexScan(eAentry, LESS_THAN, empName);
    i = 0;
    while(AM_FindNextEntry(scan) != NULL)
        i++;
    printf("Names with less than b : %d\n", i);
    AM_CloseIndexScan(scan);

    empName[0] = 'b';
    empName[1] = '\0';
    scan = AM_OpenIndexScan(eAentry, GREATER_THAN, empName);
    i = 0;
    while(AM_FindNextEntry(scan) != NULL)
        i++;
    printf("Names with more than b : %d\n", i);
    AM_CloseIndexScan(scan);

    strcpy(empName, "bla");
    scan = AM_OpenIndexScan(eAentry, EQUAL, empName);
    i = 0;
    int * kappa = NULL;
    while((kappa = (int *) AM_FindNextEntry(scan) )!= NULL)
    {
        printf("%d\n", *kappa);
        i++;
    }
    printf("Names equal with bla : %d\n", i);
    AM_CloseIndexScan(scan);


    printf("record id : %d\n", recordid);

    AM_CloseIndex(eAentry);
    AM_DestroyIndex(empAge);
    return 0;
}
