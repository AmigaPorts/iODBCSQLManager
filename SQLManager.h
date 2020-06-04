#ifndef __SQLMANAGER_H__
#define __SQLMANAGER_H__

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <iodbcext.h>
#include <odbcinst.h>
#include <dlf.h>

#include <stdio.h>

#include "SQLManager_cat.h"
#include "version.h"

#define TEXT(x)     (SQLCHAR *) x
#define TEXTC(x)    (SQLCHAR) x
#define TXTLEN(x)   strlen((char *) x)
#define TXTCMP(x1,x2)   strcmp((char *) x1, (char *) x2)

#define NUMTCHAR(X) (sizeof (X) / sizeof (SQLTCHAR))

typedef SQLRETURN SQL_API (*pSQLGetInfoFunc) (SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue, SQLSMALLINT cbInfoValueMax, SQLSMALLINT * pcbInfoValue);
typedef SQLRETURN SQL_API (*pSQLAllocHandle) (SQLSMALLINT hdl_type, SQLHANDLE hdl_in, SQLHANDLE * hdl_out);
typedef SQLRETURN SQL_API (*pSQLAllocEnv) (SQLHENV * henv);
typedef SQLRETURN SQL_API (*pSQLAllocConnect) (SQLHENV henv, SQLHDBC * hdbc);
typedef SQLRETURN SQL_API (*pSQLFreeHandle) (SQLSMALLINT hdl_type, SQLHANDLE hdl_in);
typedef SQLRETURN SQL_API (*pSQLFreeEnv) (SQLHENV henv);
typedef SQLRETURN SQL_API (*pSQLFreeConnect) (SQLHDBC hdbc);

__attribute__ ((used)) static const char *version = PROGRAM_VERSION;
__attribute__ ((used)) static const char *sc = "$STACK: 512000";

void ChangeExecuteButton(struct Window *PlayerWindow, BOOL isStarted);
void ExecuteStopQuery();
extern char *GetString(LONG id);

enum SQLMGR_ERRORS
{
	SQLMGR_ERROR_NOERROR = 0,
	SQLMGR_ERROR_NOODBC,
	SQLMGR_ERROR_NOMEM,
	SQLMGR_ERROR_FILETOOLARGE,
	SQLMGR_ERROR_NOTABLECLASS
};

enum SQLMGR_RESULTS
{
	SQLMGR_RESULT_LISTBROWSER = 0,
	SQLMGR_RESULT_TABLE,
	SQLMGR_RESULT_PLAIN_TEXT,
	SQLMGR_RESULT_XML
};

enum
{
	OBJ_SQLMANAGER_WIN = 0,
	OBJ_SQLMANAGER_ROOT,
	OBJ_SQLMANAGER_NEW,
	OBJ_SQLMANAGER_OPEN,
	OBJ_SQLMANAGER_SAVE,
	OBJ_SQLMANAGER_EXECQUERY,
	OBJ_SQLMANAGER_DISCONNECT,
    OBJ_SQLMANAGER_TABLETYPE,
    OBJ_SQLMANAGER_TABLES,
    OBJ_SQLMANAGER_CONNECTION,
	OBJ_SQLMANAGER_TEXT,
	OBJ_SQLMANAGER_VSCROLL,
    OBJ_SQLMANAGER_RESULTS,
    OBJ_SQLMANAGER_INFO,
    OBJ_SQLMANAGER_DSN,
    OBJ_SQLMANAGER_USER,
	OBJ_SQLMANAGER_XC,
	OBJ_SQLMANAGER_YC,
	OBJ_SQLMANAGER_CHANGED,
	OBJ_SQLMANAGER_COPY,
	OBJ_SQLMANAGER_CUT,
	OBJ_SQLMANAGER_PASTE,
	OBJ_SQLMANAGER_UNDO,
	OBJ_SQLMANAGER_REDO,
	OBJ_SQLMANAGER_MESSAGES,
	OBJ_SQLMANAGER_CLICKTAB,
    OBJ_SQLMANAGER_NUM
};

Object *SObjects[OBJ_SQLMANAGER_NUM];

#define OBJ(x) SObjects[x]
#define GAD(x) (struct Gadget *)SObjects[x]
#define SPACE  LAYOUT_AddChild, SpaceObject, End

static STRPTR PageLabels[] = {"Results", "Messages", NULL};

struct ProcessArguments
{
	STRPTR 	 		 prQuery;		/* The query to be executed */
	struct Window 	*prWindow; 		/* Pointer to the main window */
	struct List 	*prColName;		/* the results list pointer */
	Object 			**SObjects;		/* List of all objects */
};
#endif
