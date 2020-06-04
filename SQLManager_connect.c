#define ALL_REACTION_MACROS
#include <reaction/reaction_macros.h>
#include <reaction/reaction_prefs.h>

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <proto/listbrowser.h>
#include <gadgets/string.h>
#include <images/label.h>
#include <gadgets/chooser.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/space.h>
#include <proto/label.h>
#include <proto/bitmap.h>
#include <proto/chooser.h>
#include <images/bitmap.h>
#include <proto/string.h>
#include <gadgets/button.h>

#include <classes/requester.h>

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <iodbcext.h>
#include <odbcinst.h>
#include <dlf.h>

#include <stdio.h>
#include <stdlib.h>

#include "SQLManager.h"

#include "SQLManager_connect.h"
#include "SQLManager_glue.h"
#include "SQLManager_utils.h"
#include "SQLManager_cat.h"

#include "version.h"
#include "debug.h"

#define OBJT(x) TObjects[x]
#define GADT(x) (struct Gadget *)TObjects[x]
#define SPACE    LAYOUT_AddChild, SpaceObject, End
#define TEXT(x)     (SQLCHAR *) x
#define NUMTCHAR(X) (sizeof (X) / sizeof (SQLTCHAR))

#define MAXCOLS 64

extern HDBC  hdbc;
extern HENV  henv;
extern HSTMT hstmt;
extern int   connected;
extern char user[255];

struct List DSNlist;

enum
{
	OBJ_CONNECTDSN_WIN,
	OBJ_CONNECTDSN_UID,
    OBJ_CONNECTDSN_PWD,
    OBJ_CONNECTDSN_OK,
    OBJ_CONNECTDSN_CHOOSER,
	OBJ_CONNECTDSN_NEWDSN,
    OBJ_CONNECTDSN_CANCEL,
    OBJ_CONNECTDSN_NUM
};

Object *TObjects[OBJ_CONNECTDSN_NUM];
APTR image;

int ODBC_Connect(void)
{
    if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
        return -1;

    SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);

    if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
        return -1;

    SQLSetConnectOption (hdbc, SQL_APPLICATION_NAME, (SQLULEN) TEXT ("iODBC SQL Manager"));

    return 0;
}

int ODBC_Disconnect(void)
{
    if (hstmt)
    {
        SQLCloseCursor (hstmt);
        SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
        hstmt = NULL;
    }

    if (connected)
        SQLDisconnect (hdbc);

    if (hdbc)
    {
        SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
        hdbc = NULL;
	}

    if (henv)
    {
        SQLFreeHandle (SQL_HANDLE_ENV, henv);
        henv = NULL;
	}

    return 0;
}

char *OpenConnectDSNWindow(void)
{
	Object *testDSNwin = NULL;
	struct Window *testDSNWindow = NULL;
    char *returnvalue = NULL;
    SQLTCHAR testDSN[255] = {0};

	if (ODBC_Connect()==0)
	{
	    getDSN();
	}
	else
	{
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LOADING_ODBC),"OK", NULL);
        return returnvalue;
    }

    struct Screen *tempScreen = IIntuition->LockPubScreen("Workbench");

    if (tempScreen)
    {
       testDSNwin = (Object *)WindowObject,
			WA_ScreenTitle,        GetString(MSG_CONNECT_DSN), //"Connect to an ODBC data source",
			WA_Title,              GetString(MSG_CONNECT_DSN),
            WA_DragBar,            TRUE,
            WA_CloseGadget,        TRUE,
            WA_SizeGadget,         FALSE,
            WA_DepthGadget,        TRUE,
            WA_Activate,           TRUE,
            WA_InnerWidth,         400,
            WA_InnerHeight,        220,
#if 0
			WA_StayTop,            TRUE,
#endif
            WINDOW_IconifyGadget,  FALSE,
            WINDOW_Position,       WPOS_CENTERWINDOW,
            WINDOW_Layout,         VLayoutObject,
                LAYOUT_AddChild,    HLayoutObject,
                    LAYOUT_BevelStyle,   BVS_GROUP,
                    LAYOUT_Label,        GetString(MSG_SELECT_DSN), //"Select the DSN to use",
                    LAYOUT_LabelPlace,   BVJ_TOP_CENTER,
                    LAYOUT_VertAlignment,LALIGN_CENTER,
                    LAYOUT_AddChild, ButtonObject,
						GA_RelVerify, 		TRUE,
						GA_Disabled, 		TRUE,
						BUTTON_BevelStyle, 	BVS_NONE,
                        BUTTON_RenderImage, image = BitMapObject,
                            BITMAP_Screen, tempScreen,
                            BITMAP_Masking, TRUE,
                            BITMAP_Transparent, TRUE,
                            BITMAP_SourceFile, "images/db.png",
                            BITMAP_SelectSourceFile, "images/db.png",
                            BITMAP_DisabledSourceFile, "images/db.png",
                        BitMapEnd,
                        CHILD_WeightedHeight, 0,
                        CHILD_MinHeight, 36,
                        CHILD_MinWidth, 36,
                        CHILD_MaxWidth, 36,
                    ButtonEnd,
                    CHILD_WeightedWidth, 1,
                    LAYOUT_AddChild, OBJT(OBJ_CONNECTDSN_CHOOSER) = (Object*) ChooserObject,
                        GA_ID,             OBJ_CONNECTDSN_CHOOSER,
                        GA_RelVerify,      TRUE,
                        CHOOSER_Labels,    &DSNlist,
                        CHOOSER_Selected,  0,
                    ChooserEnd,
                    CHILD_WeightedWidth, 99,
                End,
                CHILD_WeightedHeight,  5,
                LAYOUT_AddChild,       OBJT(OBJ_CONNECTDSN_WIN) = (Object *) VLayoutObject,
                    LAYOUT_BevelStyle,    BVS_GROUP,
					LAYOUT_Label,         GetString(MSG_ENTER_CREDENTIALS), //"Enter the credentials to access the server",
					LAYOUT_LabelPlace,    BVJ_TOP_CENTER,
                    LAYOUT_SpaceInner,    TRUE,
                    LAYOUT_AddChild, HLayoutObject,
                        LAYOUT_AddChild,    VLayoutObject,
                            LAYOUT_BevelStyle,    BVS_NONE,
                            LAYOUT_AddChild, OBJT(OBJ_CONNECTDSN_UID) = (Object *) StringObject ,
                                GA_ID,              OBJ_CONNECTDSN_UID,
                                GA_RelVerify,       FALSE,
                                GA_TabCycle,        TRUE,
                                STRINGA_MaxChars,   254,
                                STRINGA_MinVisible, 15,
                                STRINGA_TextVal,    "",
                            End,
                            CHILD_Label, LabelObject, LABEL_Text, GetString(MSG_USERNAME), LabelEnd,
                            CHILD_WeightedHeight,  10,
                            LAYOUT_AddChild, OBJT(OBJ_CONNECTDSN_PWD) = (Object *) StringObject ,
                                GA_ID,              OBJ_CONNECTDSN_PWD,
                                GA_RelVerify,       TRUE,
                                GA_TabCycle,        TRUE,
                                STRINGA_MaxChars,   254,
                                STRINGA_MinVisible, 15,
                                STRINGA_TextVal,    "",
                                STRINGA_HookType,   SHK_PASSWORD,
                            End,
                            CHILD_Label, LabelObject, LABEL_Text, GetString(MSG_PASSWORD), LabelEnd,
                            CHILD_WeightedHeight,  10,
                        End,
                        CHILD_WeightedHeight,  0,
                        SPACE,
                        SPACE,
                    End, //HLayoutObject
                End,
                CHILD_WeightedHeight,  85,
                LAYOUT_AddChild,       HLayoutObject,
					LAYOUT_AddChild,       OBJT(OBJ_CONNECTDSN_NEWDSN) = Button(GetString(MSG_DSN_MANAGER), OBJ_CONNECTDSN_NEWDSN),
                    CHILD_WeightedHeight,  0,
                    SPACE,
                    SPACE,
                    SPACE,
					LAYOUT_AddChild,       OBJT(OBJ_CONNECTDSN_OK) = Button(GetString(MSG_BUTTON_CONNECT), OBJ_CONNECTDSN_OK),
                    CHILD_WeightedHeight,  0,
					LAYOUT_AddChild,       OBJT(OBJ_CONNECTDSN_CANCEL) = Button(GetString(MSG_BUTTON_CANCEL), OBJ_CONNECTDSN_CANCEL),
                    CHILD_WeightedHeight,  0,
                End,
                CHILD_WeightedHeight,  5,
            End,       //HLayoutObject
        WindowEnd;     //WindowObject
    }

    if( testDSNwin )
    {
        if ((testDSNWindow = (struct Window*) IIntuition->IDoMethod(testDSNwin, WM_OPEN)))
        {
            uint32
                sigmask     = 0,
                siggot      = 0,
                result      = 0;
            uint16
                code        = 0;
            BOOL
                done        = FALSE;

#if 0 /* it cause a machine hang if i use the TAB key on UID field */
			IIntuition->ActivateGadget(GADT(OBJ_CONNECTDSN_UID), testDSNWindow, NULL);
#endif

			/* If no DSN are created in the system, disable all gadgets except Cancel and New */
			if (IsListEmpty(&DSNlist))
			{
				IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_UID), testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_PWD), testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_CHOOSER), testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_OK), testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
			}

            IIntuition->GetAttr(WINDOW_SigMask, testDSNwin, &sigmask);
            while (!done)
            {
                siggot = IExec->Wait(sigmask | SIGBREAKF_CTRL_C);
                if (siggot & SIGBREAKF_CTRL_C) done = TRUE;
                while ((result = IIntuition->IDoMethod(testDSNwin, WM_HANDLEINPUT, &code)))
                {
                    switch(result & WMHI_CLASSMASK)
                    {
                        case WMHI_CLOSEWINDOW:
                            done = TRUE;
                        break;
                        case WMHI_GADGETUP:
                            switch (result & WMHI_GADGETMASK)
                            {
                                case OBJ_CONNECTDSN_CANCEL:
                                    done = TRUE;
                                break;
								case OBJ_CONNECTDSN_NEWDSN:
								{
									int32 result=0;

									IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_CANCEL), 	testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
									IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_OK), 		testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
									IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_NEWDSN), 	testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);

									result = IDOS->SystemTags("ODBC:iODBC",
															  SYS_Asynch,   FALSE,
															  NP_Name,      "iODBC DSN Creator",
															  NP_StackSize, 128000,
															  TAG_DONE);
									if (result!=0)
										Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LAUNCHING_IODBC), "OK", NULL);

									getDSN();
									if (IsListEmpty(&DSNlist))
									{
										IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_OK), testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
										IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_CHOOSER), testDSNWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
									}
									else
									{
										IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_OK), testDSNWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
										IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_CHOOSER), testDSNWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
										IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_UID), testDSNWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
										IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_PWD), testDSNWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
									}


									IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_CANCEL), 	testDSNWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
									IIntuition->RefreshSetGadgetAttrs(GADT(OBJ_CONNECTDSN_NEWDSN), 	testDSNWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
								}
								break;
                                case OBJ_CONNECTDSN_OK:
                                {
									if (ODBC_Connect()==0)
									{
										ULONG UID, PWD;
										ULONG DSN;
										int status;
										SQLTCHAR outDSN[4096] = {0};
										short buflen;
										struct Node *selectedNode = NULL;
										struct Requester dummyRequester;

										IIntuition->InitRequester(&dummyRequester);
										IIntuition->Request(&dummyRequester, testDSNWindow);
										IIntuition->SetWindowPointer(testDSNWindow, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);

										IIntuition->GetAttr(STRINGA_TextVal, (Object *) OBJT(OBJ_CONNECTDSN_UID), &UID);
										IIntuition->GetAttr(STRINGA_TextVal, (Object *) OBJT(OBJ_CONNECTDSN_PWD), &PWD);
										IIntuition->GetAttrs(OBJT(OBJ_CONNECTDSN_CHOOSER), CHOOSER_SelectedNode, &selectedNode, TAG_DONE);
										if (NULL!=selectedNode)
										{
											IChooser->GetChooserNodeAttrs(selectedNode, CNA_UserData, &DSN, TAG_DONE);
											snprintf((char *)testDSN, 4096, "DSN=%s;UID=%s;PWD=%s", (CONST_STRPTR) DSN, (CONST_STRPTR) UID, (CONST_STRPTR) PWD);
											D(bug("testDSN=%s\n",testDSN));

											/* This is not supported by current window.class (plain intuition windows) */
											/* So it will have no effects..	                                           */
											IIntuition->SetWindowAttrs(testDSNWindow, WA_StayTop, FALSE, TAG_DONE);

											status = SQLDriverConnect(hdbc, 0, (SQLCHAR *)testDSN, SQL_NTS,
																			   (SQLCHAR *)outDSN,  4096,
																			   &buflen, SQL_DRIVER_COMPLETE);
											D(bug("status=%d\n",status));
											if (status != 0)
											{
												DriverError(testDSNWindow, henv, hdbc, SQL_NULL_HSTMT);
												ODBC_Disconnect();
												connected = 0;
											}
											else
											{
												if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS)
												{
													DriverError(testDSNWindow, henv, hdbc, SQL_NULL_HSTMT);
													ODBC_Disconnect();
													connected = 0;
												}
												else
												{
													strncpy(user, (CONST_STRPTR) UID, strlen((CONST_STRPTR) UID));
													connected = 1;
												}
											}
											done = TRUE;
										}
										IIntuition->SetWindowPointer(testDSNWindow, WA_BusyPointer, FALSE, TAG_DONE);
										IIntuition->EndRequest(&dummyRequester, testDSNWindow);
									}
									else
									{
										Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LOADING_ODBC),"OK", NULL);
										connected = 0;
									}
                                }
                                break;
                            }
                        break;
                    }
                }
            }

			FreeChooserLabels(&DSNlist);

            IIntuition->DisposeObject(testDSNwin);
            IIntuition->DisposeObject(image);
        }
        else
            return returnvalue;
    }
    else
        return returnvalue;

    return returnvalue;
}

int ListTables(Object *LBTemp, struct List *tempList,int32 ColType)
{
    int  resultCode = -1;

    //if (SQLTables(hstmt, TEXT("%"), SQL_NTS, TEXT(""), 0, TEXT(""), 0, TEXT(""), 0) != SQL_SUCCESS)
    if (SQLTables(hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0) != SQL_SUCCESS)
    {
		D(bug("Error when listing database tables (SQLTables)!\n"));
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LISTING_TABLES), "OK", NULL);
    }
    else
    {
		SQLSMALLINT	numCols = 0;
        int colNum = 0;
        SQLTCHAR colName[50];
        SQLSMALLINT colType;
        SQLULEN colPrecision;
        SQLLEN  colIndicator;
        SQLSMALLINT colScale, colNullable;
        unsigned long totalRows, totalSets;
        SQLTCHAR fetchBuffer[1024];
        char *TABLENAME = (char *)calloc(1, 1024);

        totalSets = 1;
        int sts = 0;

        do
        {
            if (SQLNumResultCols(hstmt, &numCols) != SQL_SUCCESS)
            {
				D(bug("Error when listing database tables (SQLNumResultCols)!\n"));
                Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LISTING_TABLES), "OK", NULL);
            }
            else
            {
                if (numCols == 0)
                {
                    SQLLEN nrows = 0;

                    SQLRowCount(hstmt, &nrows);
                    D(bug("Statement executed. %ld rows affected\n", nrows > 0 ? (long) nrows : 0L));
                }
                else
                {
					D(bug("Found %d columns\n",numCols));
                    if (numCols > MAXCOLS)
                    {
                        numCols = MAXCOLS;
                        D(bug("Resultset truncated to %d columns\n",MAXCOLS));
                    }
                    for (colNum = 1; colNum <= numCols; colNum ++)
                    {
                        if (SQLDescribeCol(hstmt, colNum, (SQLTCHAR *) colName, NUMTCHAR(colName), NULL,
                                           &colType, &colPrecision, &colScale, &colNullable) == SQL_SUCCESS)
                            D(bug("Colname: %s\n",colName));
                    }

                    totalRows = 0;
                    while (1)
                    {
                        int sts = SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 1);
                        if (sts == SQL_NO_DATA_FOUND)
                        {
                            break;
                        }

                        if (sts != SQL_SUCCESS)
                        {
							D(bug("Error when listing database tables (SQLFetchScroll)!\n"));
                            Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LISTING_TABLES), "OK", NULL);
                            break;
                        }

                        for (colNum = 1; colNum <= numCols; colNum++)
                        {
                            sts = SQLGetData(hstmt, colNum, SQL_C_CHAR, fetchBuffer, NUMTCHAR(fetchBuffer), &colIndicator);
                            if (sts != SQL_SUCCESS_WITH_INFO && sts != SQL_SUCCESS)
                                break;

                            if (colIndicator != SQL_NULL_DATA)
                            {
                                switch (colNum)
                                {
                                    case 3:
                                        strncpy(TABLENAME,(const char *)fetchBuffer,1024);
                                    break;
                                    case 4:
                                        switch (ColType)
                                        {
                                            case 0:
                                            {
												D(bug("fetchBuffer = %s\n", fetchBuffer));
                                                if (strcmp((const char *)fetchBuffer, "TABLE")==0)
                                                    addRow(tempList, TABLENAME);
											}
                                            break;
                                            case 1:
                                            {
												D(bug("fetchBuffer = %s\n", fetchBuffer));
												if (strcmp((const char *)fetchBuffer, "VIEW")==0)
                                                    addRow(tempList, TABLENAME);
											}
                                            break;
                                            case 2:
                                            {
												D(bug("fetchBuffer = %s\n", fetchBuffer));
                                                if (strcmp((const char *)fetchBuffer, "SYSTEM TABLE")==0)
                                                    addRow(tempList, TABLENAME);
											}
                                            break;
                                            case 3: /* add all results without any filter */
                                                addRow(tempList, TABLENAME);
                                            break;
											memset(TABLENAME, 0x0, 1024);
                                        }
                                    break;
                                }
                            }
                        }
                        totalRows++;
                    }
                }
            }
        	totalSets++;
        	resultCode = 0;
        }
        while ((sts = SQLMoreResults(hstmt)) == SQL_SUCCESS);

		if (TABLENAME) free(TABLENAME);
    }
    return resultCode;
}
