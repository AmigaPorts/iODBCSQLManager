#ifndef __SQLMANAGER_GLUE_H__
#define __SQLMANAGER_GLUE_H__

#include <proto/exec.h>
#include <proto/chooser.h>
#include <proto/listbrowser.h>
#include <proto/intuition.h>

int32 Requester(uint32 img, TEXT *title, TEXT *reqtxt, TEXT *buttons, struct Window* win);
void addChooserNode(struct List *list, CONST_STRPTR label1, CONST_STRPTR data);
void addRow(struct List *list, CONST_STRPTR label1);
int getDSN(void);
void DriverError(struct Window *win, HENV henv, HDBC hdbc,HSTMT hstmt);
int ODBC_Errors(char *where);
int CalculateColLength(SQLSMALLINT colType,SQLULEN colPrecision, SQLSMALLINT colScale);
int executeQuery(STRPTR *args UNUSED, int32 arglen UNUSED, struct ExecBase *sysbase);

#endif

