#ifndef __SQLMANAGER_CONNECT_H__
#define __SQLMANAGER_CONNECT_H__

#include <proto/exec.h>
#include <proto/chooser.h>
#include <proto/listbrowser.h>
#include <proto/intuition.h>

int ODBC_Connect(void);
int ODBC_Disconnect(void);
char *OpenConnectDSNWindow(void);
int ListTables(Object *LBTemp, struct List *tempList,int32 ColType);

#endif

