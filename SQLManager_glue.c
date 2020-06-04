#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/chooser.h>
#include <proto/requester.h>
#include <proto/listbrowser.h>
#include <gadgets/chooser.h>
#include <gadgets/button.h>

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <classes/requester.h>

#include <time.h>
#include <sys/time.h>

#include "SQLmanager.h"

#include "SQLManager_utils.h"
#include "SQLManager_glue.h"
#include "SQLManager_connect.h"
#include "SQLManager_cat.h"

#include "version.h"
#include "debug.h"

char **stringCols; 				/* The query columns */
unsigned int lastCols = 0;		/* The latest query cols number */
struct ColumnInfo *ci = NULL;				/* The column info listbrowser struct */

extern HDBC  hdbc;
extern HENV  henv;
extern HSTMT hstmt;
extern int   connected;
extern char user[255];
extern struct List DSNlist;
extern LONG RESULT_TYPE;
//extern struct List ColName;
extern struct SignalSemaphore LockSemaphore;

#define MAX_COLS 256

struct ColumnNode{
    struct Node nd;
    char ColumnTitle[100];
};

void addChooserNode(struct List *list, CONST_STRPTR label1, CONST_STRPTR data)
{
    if (NULL == list)
        return;

    struct Node *node = IChooser->AllocChooserNode(
                                        CNA_CopyText,    TRUE,
                                        CNA_Text,        label1 == NULL ? "" : label1,
                                        CNA_UserData,    data,
                                        TAG_DONE);

    if (node)
        IExec->AddTail(list, node);
}

void addRow(struct List *list, CONST_STRPTR label1)
{
    if (NULL == list)
        return;

    struct Node *node = IListBrowser->AllocListBrowserNode(1,
                                        LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, label1 == NULL ? "" : label1,
                                        TAG_DONE);

    if (node)
        IExec->AddTail(list, node);
}

int getDSN(void)
{
    SQLTCHAR dsn[33]      = {0};
    SQLTCHAR desc[255]    = {0};
    SQLTCHAR driver[1024] = {0};
    SQLSMALLINT len1, len2;
    int retcode = -1;

    IExec->NewList(&DSNlist);

    if (SQLDataSources (henv, SQL_FETCH_FIRST, dsn, NUMTCHAR (dsn), &len1, desc, NUMTCHAR (desc), &len2) == SQL_SUCCESS)
    {
        do
        {
            char *tempDSN = (char*)calloc(1,len1+1);
            char *tempDriver = (char*)calloc(1,1024);
            char tempDes[4096] = { 0 };

            strncpy(tempDSN, (const char *) dsn, len1);

            SQLSetConfigMode(ODBC_SYSTEM_DSN);
            SQLGetPrivateProfileString((const char *) dsn, "Driver", "", (char *) driver, sizeof(driver), "odbc.ini");
            SQLGetPrivateProfileString((const char *) dsn, "Description", "", (char *) tempDes, sizeof(tempDes), "odbc.ini");

            if (driver!=NULL)
                strcpy(tempDriver, (const char *) driver);
            else
                strcpy(tempDriver, "-");

            addChooserNode(&DSNlist,(CONST_STRPTR) tempDes, (CONST_STRPTR) tempDSN);

        }
        while (SQLDataSources (henv, SQL_FETCH_NEXT, dsn, NUMTCHAR (dsn), &len1, desc, NUMTCHAR (desc), &len2) == SQL_SUCCESS);
        retcode = 0;
    }

    return retcode;
}

int32 Requester( uint32 img, TEXT *title, TEXT *reqtxt, TEXT *buttons, struct Window* win )
{
    Object *req_obj;
    int32 n;

    req_obj = (Object *)IIntuition->NewObject(IRequester->REQUESTER_GetClass(), NULL,
                REQ_TitleText,  title,
                REQ_BodyText,   reqtxt,
                REQ_GadgetText, buttons,
                REQ_Image,      img,
                TAG_DONE );

    if( !req_obj ) return 0;

    n = IIntuition->IDoMethod( req_obj, RM_OPENREQ, NULL, win, NULL);
    IIntuition->DisposeObject( req_obj );

    return n;
}

void DriverError(struct Window *win, HENV henv, HDBC hdbc,HSTMT hstmt)
{
    SQLCHAR buf[250] = {0};
    SQLCHAR sqlstate[15] = {0};
    char error[300] = {0};

    /* Get statement errors */
    if (SQLError(henv, hdbc, hstmt, sqlstate, NULL, buf, 250, NULL) == SQL_SUCCESS)
    {
        snprintf(error, 300, "%s: %s",(const char *)sqlstate, (const char *)buf);
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *)error,"OK", NULL);
        return;
    }

    /* Get connection errors */
    if (SQLError(henv, hdbc, SQL_NULL_HSTMT, sqlstate, NULL, buf, 250, NULL) == SQL_SUCCESS)
    {
        snprintf(error, 300, "%s: %s",(const char *)sqlstate, (const char *)buf);
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *)error,"OK", NULL);
        return;
    }

    /* Get connection errors */
    if (SQLError(henv, SQL_NULL_HDBC, SQL_NULL_HSTMT, sqlstate, NULL, buf, 250, NULL) == SQL_SUCCESS)
    {
        snprintf(error, 300, "%s: %s",(const char *)sqlstate, (const char *)buf);
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *)error,"OK", NULL);
        return;
    }
}

int ODBC_Errors(char *where)
{
    SQLTCHAR buf[512] = {0};
    SQLTCHAR sqlstate[15] = {0};
    SQLINTEGER native_error = 0;
    int force_exit = 0;
    int i = 0;
    char ErrorMessage[4096] = {0};
    SQLRETURN sts;

    while (hstmt && i<5)
    {
        sts = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, ++i, sqlstate, &native_error, buf, NUMTCHAR(buf), NULL);
        if (!SQL_SUCCEEDED(sts))
            break;

        snprintf(ErrorMessage, 4096, "%d: %s = %s (%ld) SQLSTATE=%s\n", i, where, buf, (long) native_error, sqlstate);
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *) ErrorMessage, "OK", NULL);

        if (!TXTCMP(sqlstate, "IM003"))
            force_exit = 1;
    }

    i = 0;
    while (hstmt && i<5)
    {
        sts = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, ++i, sqlstate, &native_error, buf, NUMTCHAR(buf), NULL);
        if (!SQL_SUCCEEDED(sts))
            break;

        snprintf(ErrorMessage, 4096, "%d: %s = %s (%ld) SQLSTATE=%s\n", i, where, buf, (long) native_error, sqlstate);
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *) ErrorMessage, "OK", NULL);

        if (!TXTCMP(sqlstate, "IM003"))
            force_exit = 1;
    }

    i = 0;
    while (hstmt && i<5)
    {
        sts = SQLGetDiagRec(SQL_HANDLE_ENV, henv, ++i, sqlstate, &native_error, buf, NUMTCHAR(buf), NULL);
        if (!SQL_SUCCEEDED(sts))
            break;

        snprintf(ErrorMessage, 4096, "%d: %s = %s (%ld) SQLSTATE=%s\n", i, where, buf, (long) native_error, sqlstate);
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *) ErrorMessage, "OK", NULL);

        if (!TXTCMP(sqlstate, "IM003"))
            force_exit = 1;
    }
    return -1;

}

int CalculateColLength(SQLSMALLINT colType,SQLULEN colPrecision, SQLSMALLINT colScale)
{
	/* Calculate the display width for the column */
	size_t displayWidth;

	switch (colType)
	{
		case SQL_VARCHAR:
		case SQL_CHAR:
		case SQL_WVARCHAR:
		case SQL_WCHAR:
		case SQL_GUID:
			displayWidth = colPrecision;
		break;

		case SQL_BINARY:
			displayWidth = colPrecision * 2;
		break;

		case SQL_LONGVARCHAR:
		case SQL_WLONGVARCHAR:
		case SQL_LONGVARBINARY:
			displayWidth = 30;	/* show only first 30 */
		break;

		case SQL_BIT:
			displayWidth = 1;
		break;

		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_BIGINT:
			displayWidth = colPrecision + 1;	/* sign */
		break;

		case SQL_DOUBLE:
		case SQL_DECIMAL:
		case SQL_NUMERIC:
		case SQL_FLOAT:
		case SQL_REAL:
			displayWidth = colPrecision + 2;	/* sign, comma */
		break;

#ifdef SQL_TYPE_DATE
		case SQL_TYPE_DATE:
#endif
		case SQL_DATE:
			displayWidth = 10;
		break;

#ifdef SQL_TYPE_TIME
		case SQL_TYPE_TIME:
#endif
		case SQL_TIME:
			displayWidth = 8;
		break;

#ifdef SQL_TYPE_TIMESTAMP
		case SQL_TYPE_TIMESTAMP:
#endif
		case SQL_TIMESTAMP:
			displayWidth = 19;
			if (colScale > 0)
				displayWidth = displayWidth + colScale + 1;
			break;

		default:
			displayWidth = 0; /* skip other data types */
	}

	return displayWidth;
}

int executeQuery(STRPTR *args UNUSED, int32 arglen UNUSED, struct ExecBase *sysbase)
{
	int resultCode = -1;
    int sts = 0;
	/* rows and resultsets returned */
	unsigned long totalRows = 0, totalSets = 0;
	BOOL IS_STOPPING = FALSE;
	struct MsgPort *queryPort;	/* The process port */
	struct Message *stopMessage = NULL;
	/* Benchmark variables! */
	struct timeval tval1, tval2;
	struct tm ptm;
	time_t hours, minutes, seconds;
	char time_string[40] = {0};
	char elapsedTime[550] = {0};
	char resultText[500] = {0};
	BOOL noRows = FALSE;
	
	memset(&tval1,0x0,sizeof(struct timeval)); /* Clear tval values to avoid bogus results.. */
	memset(&tval2,0x0,sizeof(struct timeval));
	
	struct ProcessArguments *tempPrArg = (struct ProcessArguments *)IDOS->GetEntryData();

	IExec->ObtainSemaphore(&LockSemaphore);

	/* Create a message port to get signals */
	queryPort = (struct MsgPort *)IExec->AllocSysObjectTags(ASOT_PORT,
															ASOPORT_Name, "queryPort",
															TAG_DONE);
	if (!queryPort)
	{
		IExec->ReleaseSemaphore(&LockSemaphore);
		return resultCode;
	}
	IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, BUTTON_TextPen, ~0, GA_Text, GetString(MSG_STARTING_QUERY), TAG_DONE);
	memset(&ptm, 0x0, sizeof(ptm));

	ChangeExecuteButton(tempPrArg->prWindow,TRUE);

	D(bug("Query:%s\n",tempPrArg->prQuery));

    if (SQLPrepare(hstmt, (SQLTCHAR *) tempPrArg->prQuery, SQL_NTS) != SQL_SUCCESS)
    {
		ChangeExecuteButton(tempPrArg->prWindow, FALSE);
		IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, GA_Text, "Error", TAG_DONE);
        ODBC_Errors("SQLPrepare");
		usleep(100);
		IExec->ReleaseSemaphore(&LockSemaphore);
        return resultCode;
    }

    D(bug("Executing query..\n"));
    sts = SQLExecute(hstmt);
    D(bug("Status=%d\n", sts));
    if (sts == SQL_SUCCESS || sts == SQL_SUCCESS_WITH_INFO)
    {
        SQLSMALLINT numCols = 0;
        int colNum = 0;
		SQLTCHAR colName[50] = {0};
        SQLSMALLINT colType;
        SQLULEN colPrecision;
        SQLLEN  colIndicator;
        SQLSMALLINT colScale, colNullable;
		SQLTCHAR fetchBuffer[4096] = {0};
		struct Node *node = NULL;
		FILE *fp = NULL;
		size_t displayWidths[MAX_COLS];
		size_t displayWidth;
		
		totalSets = 0;
        do
        {
            if (SQLNumResultCols(hstmt, &numCols) != SQL_SUCCESS)
            {
				ChangeExecuteButton(tempPrArg->prWindow, FALSE);
				IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, GA_Text, "Error", TAG_DONE);
                ODBC_Errors("SQLNumResultCols");
            }
            else
            {
                D(bug("numCols=%d\n", numCols));
                if (numCols == 0)
                {
                    SQLLEN nrows = 0;
					
                    SQLRowCount(hstmt, &nrows);
                    D(bug("Statement executed. %ld rows affected\n", nrows > 0 ? (long) nrows : 0L));
                    snprintf(resultText, 500 , "Statement executed. %ld rows affected\n", nrows > 0 ? (long) nrows : 0L);
                    noRows = TRUE;
                }
                else
                {
                    /* free the old columns. this will avoid memory leaks */
                    for (colNum = 1; colNum <= lastCols; colNum ++)
                    {
						if (stringCols[colNum-1]) free(stringCols[colNum-1]);
					}
					if (stringCols) free(stringCols);
					/* now we can realloc the new col numbers */
					stringCols = malloc(numCols * sizeof(*stringCols));
					if (!stringCols)
					{
						ODBC_Errors("CreateColumns");
						IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, GA_Text, "Error", TAG_DONE);
						ChangeExecuteButton(tempPrArg->prWindow, FALSE);
                        SQLCloseCursor(hstmt);
						usleep(100);
						IExec->ReleaseSemaphore(&LockSemaphore);
						return resultCode;
					}

                    if (numCols > MAX_COLS)
                    {
                        numCols = MAX_COLS;
                        D(bug("Resultset truncated to %d columns\n",MAX_COLS));
                    }

                    switch (RESULT_TYPE)
                    {
						case SQLMGR_RESULT_LISTBROWSER:
						{
							IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_RESULTS), tempPrArg->prWindow, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);

							ci = IListBrowser->AllocLBColumnInfo(numCols+1,
																 LBCIA_Column, 0,
																 LBCIA_Title, "",
																 LBCIA_Width, 60,
																 LBCIA_Flags, CIF_FIXED,
																 TAG_DONE);
						}
						break;
						case SQLMGR_RESULT_PLAIN_TEXT:
						case SQLMGR_RESULT_XML:
						{
							STRPTR textfile = NULL;

							/* Filter the files based on result type */
							if (RESULT_TYPE==SQLMGR_RESULT_PLAIN_TEXT)
								textfile = ChooseFilename("#?.txt");

							if (RESULT_TYPE==SQLMGR_RESULT_XML)
								textfile = ChooseFilename("#?.xml");

							if (textfile)
							{
								D(bug("text File=%s\n",textfile));

								fp = fopen(textfile, "w");
								if (!fp)
								{
									IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, GA_Text, "Error", TAG_DONE);
									Requester(REQIMAGE_ERROR, PROGRAM_TITLE, (char *) "Error opening the output file!", "OK", NULL);
									SQLCloseCursor(hstmt);
									ChangeExecuteButton(tempPrArg->prWindow, FALSE);
									usleep(100);
									IExec->ReleaseSemaphore(&LockSemaphore);
									return resultCode;
								}
								if (RESULT_TYPE==SQLMGR_RESULT_XML)
								{
									fprintf(fp, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
									fprintf(fp, "<!DOCTYPE QueryResult>\n");
									fprintf(fp, "<QueryResult>\n");
								}
							}
							else
							{
								SQLCloseCursor(hstmt);
								IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, GA_Text, "", TAG_DONE);
								ChangeExecuteButton(tempPrArg->prWindow, FALSE);
								usleep(100);
								IExec->ReleaseSemaphore(&LockSemaphore);
								return resultCode;
							}
						}
						break;
					}
					/* Get the start time */
					gettimeofday(&tval1, NULL);

					/* if we are using XML results add a "Columns" definition */
					if (RESULT_TYPE == SQLMGR_RESULT_XML)
						fprintf(fp, "\t<columns>\n");

                    for (colNum = 1; colNum <= numCols; colNum ++)
                    {
						if (SQLDescribeCol(hstmt, colNum, (SQLTCHAR *) colName, NUMTCHAR(colName), NULL, &colType, &colPrecision, &colScale, &colNullable) == SQL_SUCCESS)
                            D(bug("Column: %d - Nome: %s\n",colNum, colName));

						stringCols[colNum-1] = (char*) calloc(50,1);
						strncpy(stringCols[colNum-1], (char*)colName, 50);

						switch (RESULT_TYPE)
						{
							case SQLMGR_RESULT_LISTBROWSER:
							{
								IListBrowser->SetLBColumnInfoAttrs(ci, LBCIA_Column, colNum,
																	   LBCIA_Title,  stringCols[colNum-1],
																	   LBCIA_Flags, CIF_DRAGGABLE,
																	   TAG_DONE);
							}
							break;
							case SQLMGR_RESULT_PLAIN_TEXT:
							{
								displayWidth = CalculateColLength(colType, colPrecision, colScale);
								if (displayWidth == 0)
									continue;

								if (displayWidth < TXTLEN (colName))
									displayWidth = TXTLEN (colName);

								if (displayWidth > NUMTCHAR (fetchBuffer) - 1)
									displayWidth = NUMTCHAR (fetchBuffer) - 1;
								displayWidths[colNum - 1] = displayWidth;

								fprintf(fp, "%-*.*s", displayWidth, displayWidth, colName);
								if (colNum < numCols)
									fprintf(fp,"|");
							}
							break;
							case SQLMGR_RESULT_XML:
							{
								fprintf(fp, "\t\t<column>%s</column>\n",colName);
							}
							break;
						}
                    }

					/* if we are using XML results close the "Columns" definition */
					if (RESULT_TYPE == SQLMGR_RESULT_XML)
					{
						fprintf(fp, "\t</columns>\n");
						fprintf(fp, "\t<rows>\n");
					}

                    /* If we are using a text output we must add a line between column headers an results */
					if (RESULT_TYPE==SQLMGR_RESULT_PLAIN_TEXT)
					{
						int i=0;
						fprintf(fp, "\n");

						for (colNum = 1; colNum <= numCols; colNum++)
						{
							for (i = 0; i < displayWidths[colNum - 1]; i++)
								fprintf(fp, "-");
								if (colNum < numCols)
									fprintf(fp, "+");
						}
						fprintf(fp,"\n");

					}

					totalRows = 0;
                    while (1)
                    {
                        char row[25] = {0};

                        int sts = SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 1);
                        if (sts == SQL_NO_DATA_FOUND)
                        {
                            break;
                        }

                        if (sts != SQL_SUCCESS)
                        {
                            ODBC_Errors("SQLFetchScroll");
							IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, GA_Text, "Error", TAG_DONE);
							ChangeExecuteButton(tempPrArg->prWindow, FALSE);
                            break;
                        }
						snprintf(row, 25, "%ld", totalRows+1);

						switch (RESULT_TYPE)
						{
							case SQLMGR_RESULT_LISTBROWSER:
							{
								node = IListBrowser->AllocListBrowserNode(colNum,
																				LBNA_Column, 0,
																				LBNCA_CopyText, TRUE,
																				LBNCA_Text, row,
																				TAG_DONE);
								if (node)
									IExec->AddTail(tempPrArg->prColName, node);
							}
							break;
						}

						/* If we are using an XML output we must add a row definition */
						if (RESULT_TYPE == SQLMGR_RESULT_XML)
							fprintf(fp, "\t\t<row>\n");

                        for (colNum = 1; colNum <= numCols; colNum++)
                        {
                            sts = SQLGetData(hstmt, colNum, SQL_C_CHAR, fetchBuffer, NUMTCHAR(fetchBuffer), &colIndicator);
                            if (sts != SQL_SUCCESS_WITH_INFO && sts != SQL_SUCCESS)
                                break;

                            if (colIndicator != SQL_NULL_DATA)
                            {
								switch (RESULT_TYPE)
								{
									case SQLMGR_RESULT_LISTBROWSER:
									{
										IListBrowser->SetListBrowserNodeAttrs(node, LBNA_Column, colNum,
																			  LBNCA_CopyText, TRUE,
																			  LBNCA_Text, fetchBuffer,
																			  TAG_DONE);
									}
									break;
									case SQLMGR_RESULT_PLAIN_TEXT:
									{
										fprintf(fp, "%-*.*s", displayWidths[colNum - 1], displayWidths[colNum - 1], fetchBuffer);
										if (colNum < numCols)
											fprintf(fp,"|");
									}
									break;
									case SQLMGR_RESULT_XML:
									{
										fprintf(fp, "\t\t\t<%s>%s</%s>\n",stringCols[colNum-1],ParseXML((STRPTR)fetchBuffer),stringCols[colNum-1]);
									}
								}
                            }
                            else
                            {
								if (RESULT_TYPE==SQLMGR_RESULT_PLAIN_TEXT)
								{
									int i=0;
									for (i = 0; i < displayWidths[colNum - 1]; i++)
										fetchBuffer[i] = TEXTC('*');
									fetchBuffer[i] = TEXTC('\0');
									fprintf(fp, "%-*.*s", displayWidths[colNum - 1], displayWidths[colNum - 1], fetchBuffer);
									if (colNum < numCols)
										fprintf(fp,"|");
								}
							}
                        }

						/* If we are using a text output we must add a carriage return */
						if (RESULT_TYPE==SQLMGR_RESULT_PLAIN_TEXT)
							fprintf(fp,"\n");

						/* If we are using an XML output we must close the row definition */
						if (RESULT_TYPE == SQLMGR_RESULT_XML)
							fprintf(fp, "\t\t</row>\n");

                        totalRows++;

                        /* Give CPU a little breath.. */
                        usleep(50);
						/* Check if the user has stopped the query */
						stopMessage = (struct Message *) IExec->GetMsg(queryPort);
						if (stopMessage)
						{
							/* got a message. let's stop all.. */
							IS_STOPPING = TRUE;
							usleep(100);
							break;
						}
                    }

					switch (RESULT_TYPE)
					{
						case SQLMGR_RESULT_LISTBROWSER:
						{
							IIntuition->RefreshSetGadgetAttrs((struct Gadget*) OBJ(OBJ_SQLMANAGER_RESULTS), tempPrArg->prWindow, NULL, LISTBROWSER_ColumnInfo, ci, LISTBROWSER_ColumnTitles, TRUE, TAG_DONE);
							IIntuition->RefreshSetGadgetAttrs((struct Gadget*) OBJ(OBJ_SQLMANAGER_RESULTS), tempPrArg->prWindow, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
							IIntuition->RefreshSetGadgetAttrs((struct Gadget*) OBJ(OBJ_SQLMANAGER_RESULTS), tempPrArg->prWindow, NULL, LISTBROWSER_Labels, tempPrArg->prColName, TAG_DONE);
						}
						break;
						case SQLMGR_RESULT_PLAIN_TEXT:
						case SQLMGR_RESULT_XML:
						{
							if (RESULT_TYPE==SQLMGR_RESULT_XML)
							{
								fprintf(fp, "\t</rows>\n");
								fprintf(fp, "</QueryResult>\n");
							}

							if (fp)
								fclose(fp);
						}
						break;
					}
                }
            }

			lastCols = numCols;
        	totalSets++;
        	resultCode = 0;
        }
        while (((sts = SQLMoreResults(hstmt)) == SQL_SUCCESS) && (IS_STOPPING==FALSE));

		D(bug("totalsets:%ld\n",totalSets));
    }
    else if (sts == SQL_ERROR) {
    }
    else if (sts == SQL_NO_DATA) {
		int result = 0;
		int32 selectedNode = -1;
		extern struct List Tables;
		
		IIntuition->GetAttrs(OBJ(OBJ_SQLMANAGER_TABLETYPE), CHOOSER_Selected, &selectedNode, TAG_DONE);
		D(bug("selectedNode=%ld\n", selectedNode));
		if (selectedNode > -1)
		{
			D(bug("Refreshing tables with selectedNode=%ld\n", selectedNode));
			//IListBrowser->FreeListBrowserList(&Tables);
			FreeBrowserNodes(&Tables);
			IExec->NewList(&Tables);
			D(bug("Calling ListTables\n"));
			result = ListTables(OBJ(OBJ_SQLMANAGER_TABLES), &Tables, selectedNode);
			D(bug("result = %d\n", result));
			if (result == 0)
			{
				D(bug("Refreshing LISTBROWSER_Labels\n"));
				IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), tempPrArg->prWindow, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), tempPrArg->prWindow, NULL, LISTBROWSER_Labels, &Tables, TAG_DONE);
			}
		}
    }
    SQLCloseCursor(hstmt);

	/* Get the end time */
	gettimeofday(&tval2, NULL);

	/* Get elapsed time in seconds */
	seconds  = tval2.tv_sec  - tval1.tv_sec;
	/* Calculate hours, minutes and seconds */
	hours = seconds / 3600;
	minutes = (seconds - hours * 3600) / 60;
	seconds = seconds % 60;

	/* Fill the tm struct */
	ptm.tm_hour = hours;
	ptm.tm_min = minutes;
	ptm.tm_sec = seconds;

	/* Format the time in an "human" form.. */
	strftime (time_string, sizeof (time_string), "%H:%M:%S", &ptm);

	if (noRows)
		snprintf(elapsedTime, 550, "%s (%s). %s", GetString(MSG_QUERY_DONE), time_string, resultText);
	else
		snprintf(elapsedTime, 550, "%s (%s). %ld %s", GetString(MSG_QUERY_DONE), time_string, totalRows, GetString(MSG_ROWS_RETURNED));
	
	IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_INFO), tempPrArg->prWindow, NULL, BUTTON_TextPen, 3, GA_Text, elapsedTime, TAG_DONE);

	/* Change the query button */
	ChangeExecuteButton(tempPrArg->prWindow, FALSE);

	/* Remove the Column Info struct */
	if (RESULT_TYPE==SQLMGR_RESULT_LISTBROWSER)
	{
		if (ci)
			IListBrowser->FreeLBColumnInfo(ci);
		ci = NULL;
	}

	/* Free the memory allocated for the process temporary data */
	if (tempPrArg)
	{
		IExec->FreeVec(tempPrArg);
		D(bug("Agruments memory freed\n"));
	}

	/* Reply the message to the main task to tell it that we have finish */
	if (stopMessage)
	{
		D(bug("ReplyMsg\n"));
		IExec->ReplyMsg(stopMessage);
	}
	/* Free the port */
	if (queryPort)
	{
		D(bug("FreeSysObject\n"));
		IExec->FreeSysObject(ASOT_PORT, queryPort);
		queryPort = 0;
	}

	/* Release the semaphore */
	IExec->ReleaseSemaphore(&LockSemaphore);

	return resultCode;
}
