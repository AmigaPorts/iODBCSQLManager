#define ALL_REACTION_MACROS
#include <reaction/reaction_macros.h>
#include <reaction/reaction_prefs.h>

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <proto/listbrowser.h>
#include <gadgets/string.h>
#include <images/label.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/texteditor.h>
#include <proto/layout.h>
#include <proto/space.h>
#include <proto/label.h>
#include <proto/dos.h>
#include <proto/bitmap.h>
#include <proto/locale.h>
#include <proto/table.h>
#include <images/bitmap.h>
#include <proto/string.h>
#include <proto/chooser.h>
#include <proto/scroller.h>
#include <proto/gadtools.h>
#include <gadgets/texteditor.h>
#include <gadgets/chooser.h>
#include <gadgets/scroller.h>
#include <classes/requester.h>
#include <intuition/icclass.h>
#include <proto/clicktab.h>
#include <gadgets/clicktab.h>

#include <proto/asl.h>
#include <proto/utility.h>
#include <libraries/asl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <iodbcext.h>
#include <odbcinst.h>
#include <dlf.h>

#include <locale.h>

#include "SQLManager.h"
#include "SQLManager_connect.h"
#include "SQLManager_utils.h"
#include "SQLManager_glue.h"
#include "SQLManager_cat.h"

#include "version.h"
#include "debug.h"

HENV  henv      = SQL_NULL_HANDLE;
HDBC  hdbc      = SQL_NULL_HANDLE;
HSTMT hstmt     = SQL_NULL_HANDLE;
int   connected = 0;
char  user[255]  = {0};

extern struct ColumnInfo *ci;

char *filename;
BOOL isNewFile = TRUE;
APTR images[10];
struct Catalog	*catalog = NULL;		/* The program catalog */
struct Locale   *locale  = NULL;		/* The locale pointer */
struct SignalSemaphore LockSemaphore; 	/* The semaphore to control the query execution */
struct Process *QueryProcess = NULL;	/* The query process */
struct MsgPort *SQLManagerPort = NULL;	/* The main message port */

const ULONG map_texted_scroller[] = {
    GA_TEXTEDITOR_Prop_DeltaFactor,	SCROLLER_ArrowDelta,
    GA_TEXTEDITOR_Prop_Entries,	    SCROLLER_Total,
    GA_TEXTEDITOR_Prop_First,       SCROLLER_Top,
    GA_TEXTEDITOR_Prop_Visible,	    SCROLLER_Visible
    };


const ULONG map_scroller_texted[] = {
	SCROLLER_ArrowDelta,	GA_TEXTEDITOR_Prop_DeltaFactor,
	SCROLLER_Total,			GA_TEXTEDITOR_Prop_Entries,
	SCROLLER_Top,			GA_TEXTEDITOR_Prop_First,
	SCROLLER_Visible,		GA_TEXTEDITOR_Prop_Visible
	};

STRPTR TableChooser[] =
{
    "Tables",
    "Views",
    "Sys Tables",
    "All",
    NULL
};

#define MENU_START MSG_MENU_FILE

struct NewMenu nm[] =
{
    /* nm_Type, nm_Label, nm_CommKey, nm_Flags, nm_MutualExclude, nm_UserData */
	{ NM_TITLE, (STRPTR) 	NULL, NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "N", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,  	(STRPTR) "L", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "S", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "A", 0, 0L, NULL },
		{ NM_ITEM, NM_BARLABEL, 	(STRPTR) NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) NULL, 0, 0L, NULL },
		{ NM_ITEM, NM_BARLABEL, 	(STRPTR) NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "Q", 0, 0L, NULL },
	{ NM_TITLE, (STRPTR) 	NULL, NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "U", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "R", 0, 0L, NULL },
		{ NM_ITEM, NM_BARLABEL, 	(STRPTR) NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "X", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "C", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "V", 0, 0L, NULL },
	{ NM_TITLE, (STRPTR) 	NULL, NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "P", 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) "T", 0, 0L, NULL },
		{ NM_ITEM, NM_BARLABEL, 	(STRPTR) NULL, 0, 0L, NULL },
		{ NM_ITEM, (STRPTR) NULL,	(STRPTR) NULL, 0, 0L, NULL },
		{ NM_SUB,  (STRPTR) NULL, 	(STRPTR) NULL, MENUTOGGLE|CHECKIT|CHECKED, ~1, NULL },
		{ NM_SUB,  (STRPTR) NULL, 	(STRPTR) NULL, MENUTOGGLE|CHECKIT, ~2, NULL },
		{ NM_SUB,  (STRPTR) NULL, 	(STRPTR) NULL, MENUTOGGLE|CHECKIT, ~4, NULL },
		{ NM_SUB,  (STRPTR) NULL, 	(STRPTR) NULL, MENUTOGGLE|CHECKIT, ~8, NULL },
	{ NM_END, NULL, NULL, 0, 0L, NULL }
};

struct List Tables;
struct List ColName;

LONG RESULT_TYPE = SQLMGR_RESULT_LISTBROWSER; /* Type of result */

BOOL bModified = FALSE;
ULONG XC=0, YC=0;

ULONG colormap[] = {2,3,0,2,2,2,1,1,0,2,3,1};

struct Window *window 	= NULL; /* The Main Window        */
Object *Win_Object		= NULL; /* The Main Window Object */
struct Menu	  *_menu 	= NULL; /* The Program Menu       */

char winTitle[255] = {0};

struct Hook idcmpHook;

void Localize( void )
{
	int i = 0;
	struct NewMenu *tempMenu = nm;

	while( tempMenu->nm_Type != NM_END )
	{
		if( tempMenu->nm_Label == NULL)
		{
			tempMenu->nm_Label =  GetString( MENU_START + i );
			i++;
		}

		tempMenu++;
	}

	TableChooser[0] = GetString(MSG_RESULTTYPE_TABLES);
	TableChooser[1] = GetString(MSG_RESULTTYPE_VIEWS);
	TableChooser[2] = GetString(MSG_RESULTTYPE_SYSTABLES);
	TableChooser[3] = GetString(MSG_RESULTTYPE_ALL);
}

static void SAVEDS ASM
idcmpFunc(REG(a0, struct Hook *h), REG(a2, APTR winobj), REG(a1, struct IntuiMessage *imsg))
{
	struct Window *hookWindow = imsg->IDCMPWindow;
	struct TagItem *tlist, *tag;

	tlist = (struct TagItem *) imsg->IAddress;
	while ((tag = IUtility->NextTagItem(&tlist) ) != NULL)
	{
		ULONG tidata = tag->ti_Data;

		switch (tag->ti_Tag)
		{
			case GA_TEXTEDITOR_UndoAvailable:
				if (tidata)
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_UNDO), hookWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
					IIntuition->OnMenu(window, FULLMENUNUM(1,0,0));
				}
				else
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_UNDO), hookWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
					IIntuition->OffMenu(window, FULLMENUNUM(1,0,0));
				}
			break;
			case GA_TEXTEDITOR_RedoAvailable:
				if (tidata)
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_REDO), hookWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
					IIntuition->OnMenu(window, FULLMENUNUM(1,1,0));
				}
				else
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_REDO), hookWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
					IIntuition->OffMenu(window, FULLMENUNUM(1,1,0));
				}
			break;
			case GA_TEXTEDITOR_AreaMarked:
				if (tidata)
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_COPY), hookWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CUT), hookWindow, NULL, GA_Disabled, FALSE, TAG_DONE);
					IIntuition->OnMenu(window, FULLMENUNUM(1,3,0));
					IIntuition->OnMenu(window, FULLMENUNUM(1,4,0));
				}
				else
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_COPY), hookWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CUT), hookWindow, NULL, GA_Disabled, TRUE, TAG_DONE);
					IIntuition->OffMenu(window, FULLMENUNUM(1,3,0));
					IIntuition->OffMenu(window, FULLMENUNUM(1,4,0));
				}
			break;
			case GA_TEXTEDITOR_HasChanged:
			{
				bModified = tidata;
				if (bModified == TRUE)
				{
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), hookWindow, NULL, BUTTON_TextPen, 4, GA_Text, "Changed", TAG_DONE);
				}
				else
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), hookWindow, NULL, GA_Text, "", TAG_DONE);

			}
			break;
			case GA_TEXTEDITOR_CursorX:
			{
				STRPTR tempX = IExec->AllocVecTags(8, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
				XC = tidata + 1;
				snprintf(tempX, 7, "X: %ld", XC);

				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_XC), hookWindow, NULL, GA_Text, tempX, TAG_DONE);
				if (tempX)
				{
					IExec->FreeVec(tempX);
					tempX = NULL;
				}
			}
			break;
			case GA_TEXTEDITOR_CursorY:
			{
				STRPTR tempY = IExec->AllocVecTags(8, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
				YC = tidata + 1;
				snprintf(tempY, 7, "Y: %ld", YC);

				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_YC), hookWindow, NULL, GA_Text, tempY, TAG_DONE);
				if (tempY)
				{
					IExec->FreeVec(tempY);
					tempY = NULL;
				}
			}
			break;
		}
	}
}

void LoadFile()
{
	struct FileRequester *FR = NULL;
	struct Requester dummyRequester;

	IIntuition->InitRequester(&dummyRequester);
	IIntuition->Request(&dummyRequester, window);
	IIntuition->SetWindowPointer(window, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);

	FR = IAsl->AllocAslRequest(ASL_FileRequest, NULL);
	if (FR)
	{
		if ( (IAsl->AslRequestTags(FR,
									   ASLFR_TitleText, 	 GetString(MSG_ASL_OPENFILE),
									   ASLFR_RejectIcons,    TRUE,
									   ASLFR_DoPatterns,     TRUE,
									   ASLFR_SleepWindow,    TRUE,
									   ASLFR_InitialPattern, (ULONG) "#?.sql",
									   TAG_DONE) ) == FALSE)
		{
			D(bug("User has click on Cancel\n"));
		}
		else
		{
			filename = IExec->AllocVecTags(strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
			if (filename)
			{
				D(bug("file:%s%s\n",FR->fr_Drawer,FR->fr_File));
				snprintf(filename, strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, "%s%s",FR->fr_Drawer, FR->fr_File);
				if (FileExists(filename) == TRUE)
				{
					if (ReadTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename) == 0)
					{
						IIntuition->SetGadgetAttrs(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GA_TEXTEDITOR_HasChanged, FALSE, TAG_DONE);
						bModified = FALSE;
						isNewFile = FALSE;
						IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);

						/* if all is ok, let's update the window title */
						memset(winTitle,0x0,255);
						snprintf(winTitle, 254, "SQL Manager - %s", filename);
						IIntuition->SetWindowTitles(window, winTitle, (CONST_STRPTR) -1);
					}
					else
						Requester(REQIMAGE_QUESTION, PROGRAM_TITLE, GetString(MSG_ERROR_READING_FILE),"OK", window);
					}
				}
			}
		}
		IAsl->FreeAslRequest(FR);
		IIntuition->SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
		IIntuition->EndRequest(&dummyRequester, window);
}

void SaveFile(BOOL SaveAs)
{
	   struct FileRequester *FR = NULL;
	   struct Requester dummyRequester;

	   IIntuition->InitRequester(&dummyRequester);
	   IIntuition->Request(&dummyRequester, window);
	   IIntuition->SetWindowPointer(window, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);

	   if ((isNewFile == TRUE) || (SaveAs == TRUE))
	   {
		   FR = IAsl->AllocAslRequest(ASL_FileRequest, NULL);
		   if (FR)
		   {
			   TEXT *path;

			   path = IDOS->PathPart(filename);
			   *path = '\0';

			   if ( (IAsl->AslRequestTags(FR,
									  ASLFR_TitleText, 	    (ULONG) GetString(MSG_ASL_SAVEFILE),
									  ASLFR_RejectIcons,    TRUE,
									  ASLFR_DoPatterns,     TRUE,
									  ASLFR_SleepWindow,    TRUE,
									  ASLFR_InitialDrawer,  (ULONG) path,
									  ASLFR_DoSaveMode,     TRUE,
									  ASLFR_InitialFile,    (ULONG) filename,
									  TAG_DONE) ) == FALSE)
			   {
						D(bug("User has click on Cancel\n"));
			   }
			   else
			   {
				   filename = IExec->AllocVecTags(strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
				   if (filename)
				   {
					   D(bug("file:%s%s\n",FR->fr_Drawer,FR->fr_File));
					   snprintf(filename, strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, "%s%s",FR->fr_Drawer, FR->fr_File);
					   if (FileExists(filename) == TRUE)
					   {
							   if (Requester(REQIMAGE_QUESTION, PROGRAM_TITLE, GetString(MSG_ASL_FILEEXISTS),GetString(MSG_YESNO), window) == 1)
							   {
									   SaveTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename);
									   IIntuition->SetGadgetAttrs(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GA_TEXTEDITOR_HasChanged, FALSE, TAG_DONE);
									   bModified = FALSE;
									   isNewFile = FALSE;
									   IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);

									   /* if all is ok, let's update the window title */
									   memset(winTitle,0x0,255);
									   snprintf(winTitle, 254, "SQL Manager - %s", filename);
									   IIntuition->SetWindowTitles(window, winTitle, (CONST_STRPTR) -1);
							   }
					   }
					   else
					   {
						   SaveTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename);
						   IIntuition->SetGadgetAttrs(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GA_TEXTEDITOR_HasChanged, FALSE, TAG_DONE);
						   bModified = FALSE;
						   isNewFile = FALSE;
						   IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);

						   /* if all is ok, let's update the window title */
						   memset(winTitle,0x0,255);
						   snprintf(winTitle, 254, "SQL Manager - %s", filename);
						   IIntuition->SetWindowTitles(window, winTitle, (CONST_STRPTR) -1);
			 		   }
					}
				}
				IAsl->FreeAslRequest(FR);
			}
		}
		else
		{
			SaveTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename);
			IIntuition->SetGadgetAttrs(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GA_TEXTEDITOR_HasChanged, FALSE, TAG_DONE);
			bModified = FALSE;
			isNewFile = FALSE;
			IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);
		}

		IIntuition->SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
		IIntuition->EndRequest(&dummyRequester, window);
}

void ChangeExecuteButton(struct Window *PlayerWindow, BOOL isStarted)
{
	Object *temp = NULL;
	struct Screen *tempScreen = IIntuition->LockPubScreen("Workbench");

	/* If we are stopping the query disable the Stop menu item */
	if (isStarted == FALSE)
	{
		IIntuition->OnMenu(window, FULLMENUNUM(2,0,0));
		IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));
		IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_EXECQUERY), GA_HintInfo, GetString(MSG_SB_BUTTON_START_QUERY), TAG_DONE);
	}
	else
	{
	/* otherwise enable it */
		IIntuition->OffMenu(window, FULLMENUNUM(2,0,0));
		IIntuition->OnMenu(window, FULLMENUNUM(2,1,0));
		IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_EXECQUERY), GA_HintInfo, GetString(MSG_SB_BUTTON_STOP_QUERY), TAG_DONE);
	}

	/* Change the play/stop bitmap button according the isStarted value */
	IIntuition->SetGadgetAttrs(GAD(OBJ_SQLMANAGER_ROOT), PlayerWindow, NULL,
		LAYOUT_ModifyChild, OBJ(OBJ_SQLMANAGER_EXECQUERY),
			CHILD_ReplaceObject, 					temp = ButtonObject,
					GA_ID, 							OBJ_SQLMANAGER_EXECQUERY,
					GA_ReadOnly, 					FALSE,
					GA_RelVerify, 					TRUE,
					BUTTON_BevelStyle, 				BVS_THIN,
					BUTTON_RenderImage, 			images[8] =  BitMapObject, 		//Pause
							BITMAP_Screen, 				tempScreen,
							BITMAP_Masking, 			TRUE,
							BITMAP_Transparent, 		TRUE,
							BITMAP_SourceFile, 			isStarted == FALSE ? "TBimages:player"		: "TBImages:stop",
							BITMAP_SelectSourceFile, 	isStarted == FALSE ? "TBimages:player_s"	: "TBImages:stop_s",
							BITMAP_DisabledSourceFile, 	isStarted == FALSE ? "TBimages:player_g"	: "TBImages:stop_g",
						BitMapEnd,
					ButtonEnd,
		TAG_DONE);

	OBJ(OBJ_SQLMANAGER_EXECQUERY) = temp;
	IIntuition->UnlockPubScreen(NULL, tempScreen);
	ILayout->RethinkLayout(GAD(OBJ_SQLMANAGER_ROOT), PlayerWindow, NULL, TRUE);
}

void ExecuteStopQuery()
{
	struct Task *queryTask = NULL;

	IExec->Forbid();
	queryTask = IExec->FindTask("SQL Manager Query");
	IExec->Permit();

	/* if the query is not started, let's start it */
	if (!queryTask)
	{
		/* First of all, wait for the semaphore to be released from the query process (if any..) */
		IExec->ObtainSemaphore(&LockSemaphore);

		UBYTE *SQLQuery;
		if ( (SQLQuery = (UBYTE *) IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GM_TEXTEDITOR_ExportText, NULL)) )
		{
			if (strcmp((const char*)SQLQuery,"")!=0)
			{
				struct ProcessArguments *ProcArg = NULL;

				/* change the button/menu status */
				ChangeExecuteButton(window, TRUE);
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_INFO), window, NULL, BUTTON_TextPen, ~0, GA_Text, GetString(MSG_STARTING_QUERY), TAG_DONE);

				/* Remove the old records from listbrowser */
				FreeBrowserNodes(&ColName);
				IExec->NewList(&ColName);
				/* Refresh the list browser to show no rows and no columns */
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_RESULTS), window, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_RESULTS), window, NULL, LISTBROWSER_Labels, &ColName, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_RESULTS), window, NULL, LISTBROWSER_ColumnInfo, NULL, TAG_DONE);

				/* Let's sleep a little bit to refresh the listbrowser */
				usleep(100);

				/* Execute the query in a separate task */
				ProcArg = IExec->AllocVecTags(sizeof(struct ProcessArguments), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
				if (ProcArg)
				{
					ProcArg->prQuery 		= (STRPTR)SQLQuery;	/* the query to be executed */
					ProcArg->prWindow 		= window;			/* the window pointer */
					ProcArg->prColName 		= &ColName;			/* the results list pointer */
					ProcArg->SObjects		= SObjects;			/* List of all objects */
					QueryProcess = (struct Process *)IDOS->CreateNewProcTags(
														NP_Entry,		executeQuery,
														NP_Name,		(ULONG) "SQL Manager Query",
														NP_Child,		TRUE,
														NP_StackSize, 	65536,
														NP_EntryData, 	ProcArg,
														TAG_DONE);
				}
				else
				{
					Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_NOMEMFORQUERY),"OK", NULL);
					/* change the button/menu status */
					ChangeExecuteButton(window, FALSE);
					IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_INFO), window, NULL, BUTTON_TextPen, ~0, GA_Text, "", TAG_DONE);
				}
			}
		}
		IExec->ReleaseSemaphore(&LockSemaphore);
	}
	else
	{
		struct Message *stopMessage = NULL;

		/* Disable the Play/Stop button so we are sure that nobody can press it again.. */
		IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, TRUE, TAG_DONE);
		/* Show stopping query.. */
		IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_INFO), window, NULL, BUTTON_TextPen, ~0, GA_Text, GetString(MSG_STOPPING_QUERY), TAG_DONE);

		stopMessage = IExec->AllocVecTags(sizeof(struct Message), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
		if (stopMessage)
		{
			struct MsgPort *queryPort= NULL;

			/* Fill in all message info */
			stopMessage->mn_Node.ln_Type 	= NT_MESSAGE;
			stopMessage->mn_Length 			= sizeof(struct Message);
			stopMessage->mn_ReplyPort 		= SQLManagerPort;

			D(bug("Forbid\n"));
			/* Send a stop message to the port */
			IExec->Forbid();
			queryPort = IExec->FindPort("queryPort"); /* try to find the queryPort so we can send a message to it */
			if (queryPort)
				IExec->PutMsg(queryPort, stopMessage);
			IExec->Permit();
			D(bug("Permit\n"));
			D(bug("WaitPort\n"));
			IExec->WaitPort(SQLManagerPort);
			while ((stopMessage = IExec->GetMsg(SQLManagerPort))); /* Remove all the replies from the queue */
			D(bug("Done\n"));

			/* Obtain the semaphore. so we are sure that the query has been stopped */
			IExec->ObtainSemaphore(&LockSemaphore);
			/* if we are stopping the query, change the button/menu status */
			IIntuition->OnMenu(window, FULLMENUNUM(2,0,0));
			IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));
			/* Change also the hint info */
			IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_EXECQUERY), GA_HintInfo, GetString(MSG_SB_BUTTON_START_QUERY), TAG_DONE);
			/* re enable the play button now that we are sure that our process is ready again */
			IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, FALSE, TAG_DONE);
			/* release the semaphore */
			IExec->ReleaseSemaphore(&LockSemaphore);
			/* free the message memory */
			IExec->FreeVec(stopMessage);
		}
		else
			Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_NOMEMFORQUERY),"OK", NULL);
	}
}

BOOL TestAndExit(BOOL bClearEditor)
{
	struct Requester dummyRequester;
	int nResult = 0;
	BOOL tOk = FALSE;

	nResult = Requester(REQIMAGE_QUESTION, PROGRAM_TITLE, GetString(MSG_QUERY_CHANGED),GetString(MSG_YESNOCANCEL), window);
	switch (nResult)
	{
		case 1: /* User has clicked "Yes" */
		{
			struct FileRequester *FR = NULL;

			IIntuition->InitRequester(&dummyRequester);
			IIntuition->Request(&dummyRequester, window);
			IIntuition->SetWindowPointer(window, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);

			if (isNewFile == TRUE)
			{
				FR = IAsl->AllocAslRequest(ASL_FileRequest, NULL);
				if (FR)
				{
					TEXT *path;

					path = IDOS->PathPart(filename);
					*path = '\0';

					if ( (IAsl->AslRequestTags(FR,
										   ASLFR_TitleText, 	 (ULONG) GetString(MSG_ASL_SAVEFILE),
										   ASLFR_RejectIcons,    TRUE,
										   ASLFR_DoPatterns,     TRUE,
										   ASLFR_SleepWindow,    TRUE,
										   ASLFR_InitialDrawer,  (ULONG) path,
										   ASLFR_DoSaveMode,     TRUE,
										   ASLFR_InitialFile,    (ULONG) filename,
										   TAG_DONE) ) == FALSE)
					{
						D(bug("User has click on Cancel\n"));
						tOk = FALSE;
					}
					else
					{
						filename = IExec->AllocVecTags(strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
						if (filename)
						{
							D(bug("file:%s%s\n",FR->fr_Drawer,FR->fr_File));
							snprintf(filename, strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, "%s%s",FR->fr_Drawer, FR->fr_File);
							if (FileExists(filename) == TRUE)
							{
								if (Requester(REQIMAGE_QUESTION, PROGRAM_TITLE, GetString(MSG_ASL_FILEEXISTS),GetString(MSG_YESNO), window) == 1)
								{
									SaveTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename);
									bModified = FALSE;
									IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);
									tOk = TRUE;
								}
							}
							else
							{
								SaveTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename);
								bModified = FALSE;
								IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);
								tOk = TRUE;
							}
						}
					}
					IAsl->FreeAslRequest(FR);
				}
			}
			else
			{
				SaveTEFile(GAD(OBJ_SQLMANAGER_TEXT), window, filename);
				bModified = FALSE;
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CHANGED), window, NULL, GA_Text, "", TAG_DONE);
				tOk = TRUE;
			}
			IIntuition->SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
			IIntuition->EndRequest(&dummyRequester, window);

			if (tOk == TRUE)
			{
				if (bClearEditor == TRUE)
					IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT),
											   window,
											   NULL,
											   GM_TEXTEDITOR_ClearText,
											   NULL);
			}
		}
		break;
		case 2: /* User has clicked "No" */
		{
			tOk = TRUE;
			if (bClearEditor == TRUE)
				IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT),
										   window,
										   NULL,
										   GM_TEXTEDITOR_ClearText,
										   NULL);
		}
		break;
	}
	return tOk;
}

int main( void )
{
	ULONG signal;
    ULONG done = FALSE;
    struct Screen *tempScreen = IIntuition->LockPubScreen("Workbench");
	Class *TableClass;
	
	setlocale(LC_ALL,"");

    if (ODBC_Connect()!=0)
    {
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LOADING_ODBC),"OK", NULL);
        exit(SQLMGR_ERROR_NOODBC);
    }
    ODBC_Disconnect(); /* we will connect to DSN later on DSN window */

	/* Create a message port to get signals */
	SQLManagerPort = (struct MsgPort *)IExec->AllocSysObjectTags(ASOT_PORT,
		 														 ASOPORT_Name, "SQLManagerPort",
																 TAG_DONE);
	if (!SQLManagerPort)
	{
        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, "Cannot create a message port!","OK", NULL);
        exit(SQLMGR_ERROR_NOMEM);
	}

	/* Initialize the Semaphore */
	memset(&LockSemaphore, 0, sizeof(LockSemaphore));
	IExec->InitSemaphore(&LockSemaphore);

	catalog = ILocale->OpenCatalogA( NULL, "SQLManager.catalog", NULL );
	locale  = ILocale->OpenLocale( NULL );

	filename = IExec->AllocVecTags(strlen("NoName.sql")+1, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
	if (!filename)
    {
		Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERR_OUTOFMEM),"OK", NULL);
		exit(SQLMGR_ERROR_NOMEM);
    }
	else
		strcpy(filename,"NoName.sql");

	snprintf(winTitle, 254, "SQL Manager - %s", filename);

    Object
            *page1 = NULL, /* The page 1 is the page that contains the ResultSets */
            *page2 = NULL; /* The page 2 is the page that contains the Messages   */

	/* 	Initializing TableClass
		TableClass is used for Messaging display and also to show resulset in an HTML Table.
		If we don't find the class we must exit. So it needs OS4.1 UPD3 to works correctly
	*/
	struct ClassLibrary *TableBase;
	uint32 tableError;
	TableBase = (struct ClassLibrary *) IIntuition->OpenClass("images/table.image", 53, &TableClass);
	if (TableBase == NULL)
	{
		if (filename)
		{
			IExec->FreeVec(filename);
			filename = NULL;
		}

		if (locale)
		{
			ILocale->CloseLocale( locale );
			locale = NULL;
		}
		if (catalog)
		{
			ILocale->CloseCatalog( catalog );
			catalog = NULL;
		}

		if (SQLManagerPort)
		{
			IExec->FreeSysObject(ASOT_PORT, SQLManagerPort);
			SQLManagerPort = 0;
		}		

        Requester(REQIMAGE_ERROR, PROGRAM_TITLE, "Cannot open TableClass!","OK", NULL);
        exit(SQLMGR_ERROR_NOTABLECLASS);
	}

    page1 = (Object *) VLayoutObject,
        LAYOUT_BevelStyle,    BVS_NONE,
        LAYOUT_SpaceInner,    FALSE,
        LAYOUT_AddChild, HLayoutObject,
			LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_RESULTS) = ListBrowserObject,
				GA_ID, 						OBJ_SQLMANAGER_RESULTS,
				GA_RelVerify,               TRUE,
				LISTBROWSER_ColumnTitles,   TRUE,
				LISTBROWSER_Separators,     TRUE,
				LISTBROWSER_AutoFit,        TRUE,
				LISTBROWSER_HorizontalProp, TRUE,
			End,
		End,
	End;

    page2 = (Object *) VLayoutObject,
        LAYOUT_BevelStyle,    BVS_NONE,
        LAYOUT_SpaceInner,    FALSE,
        LAYOUT_AddChild, HLayoutObject,
			LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_MESSAGES) = IIntuition->NewObject(TableClass, NULL,
				TABLE_NumberOfColumns,		1,
				TABLE_CurrentColumn,		1,
				TABLE_ErrorCode,			&tableError,
				GA_ID, 						OBJ_SQLMANAGER_MESSAGES,
				GA_RelVerify,               TRUE,
			End,
		End,
	End;

    if (tempScreen)
    {
        Win_Object = WindowObject,
			WA_ScreenTitle, 	"SQL Manager Copyright 2010, AmigaSoft",
			WA_SizeGadget, 		TRUE,
			WA_Left, 			40,
			WA_Top, 			30,
			WA_InnerWidth,      950,
			WA_InnerHeight,     500,
			WA_DepthGadget, 	TRUE,
			WA_DragBar, 		TRUE,
			WA_CloseGadget, 	TRUE,
			WA_Activate, 		TRUE,
			WA_RMBTrap,	        FALSE,
			WA_ReportMouse,	    TRUE,
			WA_IDCMP,           IDCMP_CLOSEWINDOW | IDCMP_ACTIVEWINDOW | IDCMP_CHANGEWINDOW | IDCMP_MENUPICK,
			WINDOW_GadgetHelp,	TRUE,
			WINDOW_IDCMPHook,   (ULONG) &idcmpHook,
			WINDOW_IDCMPHookBits, IDCMP_IDCMPUPDATE,
			WINDOW_ParentGroup,  VLayoutObject,
                LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_ROOT) = HLayoutObject,
                    LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_NEW) = ButtonObject,
                        //GA_Text, "New",
						GA_ID,             OBJ_SQLMANAGER_NEW,
						GA_RelVerify, 	   TRUE,
						GA_HintInfo,	   GetString(MSG_SB_BUTTON_NEW),
                        BUTTON_BevelStyle, BVS_THIN,
						BUTTON_RenderImage, images[0] = BitMapObject,
                            BITMAP_Screen, tempScreen,
                            BITMAP_Masking, TRUE,
                            BITMAP_Transparent, TRUE,
                            BITMAP_SourceFile, "TBImages:new",
							BITMAP_SelectSourceFile, "TBImages:new_s",
							BITMAP_DisabledSourceFile, "TBImages:new_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
                    LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_OPEN) = ButtonObject,
                        //GA_Text, "Open",
						GA_ID,             OBJ_SQLMANAGER_OPEN,
						GA_HintInfo,	   GetString(MSG_SB_BUTTON_OPEN),
						GA_RelVerify, 	   TRUE,
                        BUTTON_BevelStyle, BVS_THIN,
						BUTTON_RenderImage, images[1] = BitMapObject,
                            BITMAP_Screen, tempScreen,
                            BITMAP_Masking, TRUE,
                            BITMAP_Transparent, TRUE,
                            BITMAP_SourceFile, "TBImages:open",
							BITMAP_SelectSourceFile, "TBImages:open_s",
							BITMAP_DisabledSourceFile, "TBImages:open_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_SAVE) = ButtonObject,
						GA_ID,              OBJ_SQLMANAGER_SAVE,
						GA_RelVerify, 		TRUE,
						GA_HintInfo,	   GetString(MSG_SB_BUTTON_SAVE),
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage, images[2] = BitMapObject,
							BITMAP_Screen, 	tempScreen,
							BITMAP_Masking, 			TRUE,
							BITMAP_Transparent, 		TRUE,
							BITMAP_SourceFile, 			"TBImages:save",
							BITMAP_SelectSourceFile, 	"TBImages:save_s",
							BITMAP_DisabledSourceFile,  "TBImages:save_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
                    SPACE,
                    CHILD_WeightedWidth, 1,
					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_CUT) = ButtonObject,
						GA_ID,				OBJ_SQLMANAGER_CUT,
						GA_RelVerify, 		TRUE,
						GA_Disabled,        TRUE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_CUT),
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage,	images[3] = BitMapObject,
							BITMAP_Screen,  			tempScreen,
							BITMAP_Masking, 			TRUE,
							BITMAP_Transparent, 		TRUE,
							BITMAP_SourceFile, 			"TBImages:cut",
							BITMAP_SelectSourceFile, 	"TBImages:cut_s",
							BITMAP_DisabledSourceFile, 	"TBImages:cut_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
  					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_COPY) = ButtonObject,
                        //GA_Text, "Copy",
						GA_ID,        		OBJ_SQLMANAGER_COPY,
						GA_RelVerify, 		TRUE,
						GA_Disabled,  		TRUE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_COPY),
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage, images[4] = BitMapObject,
                            BITMAP_Screen, tempScreen,
                            BITMAP_Masking, TRUE,
                            BITMAP_Transparent, TRUE,
                            BITMAP_SourceFile, "TBImages:copy",
                            BITMAP_SelectSourceFile, "TBImages:copy_s",
                            BITMAP_DisabledSourceFile, "TBImages:copy_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_PASTE) = ButtonObject,
						GA_ID,              OBJ_SQLMANAGER_PASTE,
						GA_RelVerify, 		TRUE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_PASTE),
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage, images[5] = BitMapObject,
							BITMAP_Screen,  tempScreen,
							BITMAP_Masking, 		   TRUE,
							BITMAP_Transparent, 	   TRUE,
							BITMAP_SourceFile, 		   "TBImages:paste",
							BITMAP_SelectSourceFile,   "TBImages:paste_s",
                            BITMAP_DisabledSourceFile, "TBImages:paste_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
                    SPACE,
                    CHILD_WeightedWidth, 1,

					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_UNDO) = ButtonObject,
                        //GA_Text, "Copy",
						GA_ID,        		OBJ_SQLMANAGER_UNDO,
						GA_RelVerify, 		TRUE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_UNDO),
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage, images[6] = BitMapObject,
							BITMAP_Screen,  tempScreen,
							BITMAP_Masking, 		    TRUE,
							BITMAP_Transparent, 	    TRUE,
							BITMAP_SourceFile, 		   "TBImages:undo",
							BITMAP_SelectSourceFile,   "TBImages:undo_s",
							BITMAP_DisabledSourceFile, "TBImages:undo_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_REDO) = ButtonObject,
						GA_ID,              OBJ_SQLMANAGER_REDO,
						GA_RelVerify, 		TRUE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_REDO),
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage, images[7] = BitMapObject,
							BITMAP_Screen,  tempScreen,
							BITMAP_Masking, 		   TRUE,
							BITMAP_Transparent, 	   TRUE,
							BITMAP_SourceFile, 		   "TBImages:redo",
							BITMAP_SelectSourceFile,   "TBImages:redo_s",
							BITMAP_DisabledSourceFile, "TBImages:redo_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,

                    SPACE,
                    CHILD_WeightedWidth, 1,
					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_EXECQUERY) = ButtonObject,
						GA_ID, 				OBJ_SQLMANAGER_EXECQUERY,
						GA_RelVerify, 		TRUE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_START_QUERY),
						GA_Disabled,        TRUE,
						BUTTON_BevelStyle, 	BVS_THIN,
						BUTTON_RenderImage, images[8] = BitMapObject,
						  BITMAP_Screen, 	tempScreen,
							BITMAP_Masking, 		   TRUE,
							BITMAP_Transparent, 	   TRUE,
							BITMAP_SourceFile, 		   "TBImages:player",
							BITMAP_SelectSourceFile,   "TBImages:player_s",
                            BITMAP_DisabledSourceFile, "TBImages:player_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
                    CHILD_WeightedWidth, 1,
					SPACE,
                    CHILD_WeightedWidth, 1,
					LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_DISCONNECT) = ButtonObject,
						GA_ID,              OBJ_SQLMANAGER_DISCONNECT,
						GA_RelVerify,       TRUE,
						GA_Selected,        FALSE,
						GA_HintInfo,	    GetString(MSG_SB_BUTTON_DISCONNECT_DB),
						GA_ToggleSelect,    TRUE,
						BUTTON_BevelStyle,  BVS_THIN,
						BUTTON_RenderImage, images[9] = BitMapObject,
						  BITMAP_Screen,    tempScreen,
							BITMAP_Masking, 		   TRUE,
							BITMAP_Transparent, 	   TRUE,
							BITMAP_SourceFile, 		   "TBImages:dboff",
							BITMAP_SelectSourceFile,   "TBImages:dboff_s",
							BITMAP_DisabledSourceFile, "TBImages:dboff_g",
                        BitMapEnd,
                    ButtonEnd,
                    CHILD_WeightedHeight, 0,
                    CHILD_MinHeight, 36,
                    CHILD_MinWidth, 36,
                    CHILD_MaxWidth, 36,
					SPACE,
					CHILD_WeightedWidth, 1,
                    SPACE,
					CHILD_WeightedWidth, 98,
                End, // (1)
                CHILD_WeightedHeight, 0,
                LAYOUT_AddChild, HLayoutObject, // (2)
                    LAYOUT_AddChild, VLayoutObject, // (3)
                        LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_TABLETYPE) = (Object*) ChooserObject,
                            GA_ID,             		OBJ_SQLMANAGER_TABLETYPE,
							GA_RelVerify,      		TRUE,
                            CHOOSER_LabelArray,    	TableChooser,
                            CHOOSER_Selected,  		3,
                        ChooserEnd,
                        CHILD_WeightedWidth, 99,
                        LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_TABLES) = ListBrowserObject,
                                GA_ID,                    OBJ_SQLMANAGER_TABLES,
                                GA_RelVerify,             TRUE,
                                LISTBROWSER_Separators,   TRUE,
                                LISTBROWSER_Hierarchical, TRUE,
                                LISTBROWSER_ShowSelected, TRUE,
                        End,
                    End, // End of (3)
                    CHILD_WeightedWidth, 20,
                    LAYOUT_AddChild, VLayoutObject, // Start of (4)
                        LAYOUT_AddChild, HLayoutObject,
							LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_TEXT) = TextEditorObject,
								GA_ID,                    		OBJ_SQLMANAGER_TEXT,
								ICA_TARGET,	                    ICTARGET_IDCMP,
								GA_RelVerify,               	TRUE,
								GA_Selected,                    TRUE,
								GA_TEXTEDITOR_TabToSpaces,		TRUE,
								GA_TEXTEDITOR_CursorBlinkSpeed, 5,
								GA_TEXTEDITOR_ColorMap, 		colormap,
								GA_TEXTEDITOR_BevelStyle,       BVS_THIN,
                            End,
                            LAYOUT_AddChild,
								OBJ(OBJ_SQLMANAGER_VSCROLL) = ScrollerObject,
									GA_ID, 					OBJ_SQLMANAGER_VSCROLL,
									ICA_TARGET,	            ICTARGET_IDCMP,
									SCROLLER_Orientation, 	SORIENT_VERT,
								//End,
							End,
                        End,
						CHILD_WeightedHeight, 40,
                        LAYOUT_AddChild, HLayoutObject,
							LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_CLICKTAB) = ClickTabObject,
								GA_Text,            PageLabels,
								CLICKTAB_Current,   0,
								CLICKTAB_PageGroup, PageObject,
									PAGE_Add,       page1,
									PAGE_Add,       page2,
								End,
							End,
                        End,
						CHILD_WeightedHeight, 60,
                        LAYOUT_AddChild, HLayoutObject,
                            LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_INFO) = ButtonObject,
								GA_ID,              OBJ_SQLMANAGER_INFO,
                                GA_Text,            "",
								GA_HintInfo,	    GetString(MSG_STS_STATUS_HI),
                                BUTTON_BevelStyle,    BVS_FIELD,
                                BUTTON_Justification, BCJ_LEFT,
                            End,
                            LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_DSN) = ButtonObject,
								GA_ID,              OBJ_SQLMANAGER_DSN,
                                GA_Text,            "",
								GA_HintInfo,	    GetString(MSG_STS_DBNAME_HI),
                                BUTTON_BevelStyle,  BVS_FIELD,
                            End,
                            CHILD_MaxWidth,         140,
                            LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_CONNECTION) = ButtonObject,
                                GA_ID,              OBJ_SQLMANAGER_CONNECTION,
                                GA_Text,            GetString(MSG_NOT_CONNECTED),
								GA_HintInfo,	    GetString(MSG_STS_CONNSTATUS_HI),
                                BUTTON_TextPen,     4,
                                BUTTON_BevelStyle,  BVS_FIELD,
                            End,
                            CHILD_MaxWidth,         120,
                            LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_USER) = ButtonObject,
                                GA_ID,              OBJ_SQLMANAGER_USER,
                                GA_Text,            "",
								GA_HintInfo,	    GetString(MSG_STS_CURRENTUSER_HI),
                                BUTTON_BevelStyle,  BVS_FIELD,
                            End,
                            CHILD_MaxWidth,         60,
							LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_XC) = ButtonObject,
								GA_ID,              OBJ_SQLMANAGER_XC,
                                GA_Text,            "",
								GA_HintInfo,	    GetString(MSG_STS_CURX_HI),
                                BUTTON_BevelStyle,  BVS_FIELD,
                            End,
                            CHILD_MaxWidth,         60,
							LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_YC) = ButtonObject,
								GA_ID,              OBJ_SQLMANAGER_YC,
                                GA_Text,            "",
								GA_HintInfo,	    GetString(MSG_STS_CURY_HI),
                                BUTTON_BevelStyle,  BVS_FIELD,
                            End,
                            CHILD_MaxWidth,         60,
							LAYOUT_AddChild, OBJ(OBJ_SQLMANAGER_CHANGED) = ButtonObject,
								GA_ID,              OBJ_SQLMANAGER_CHANGED,
                                GA_Text,            "",
                                GA_HintInfo,		GetString(MSG_STS_TEXTCHANGED_HI),
                                BUTTON_BevelStyle,  BVS_FIELD,
                            End,
							CHILD_MaxWidth,         80,
                        End,
                        CHILD_WeightedHeight, 0,
                    End, // End of (4)
                    CHILD_WeightedWidth, 80,
                End, // End of (2)
            End,
        EndWindow;
        IIntuition->UnlockPubScreen(NULL,tempScreen);
    }

    if( Win_Object )
    {
		int i = 0;
		//IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_VSCROLL), ICA_TARGET, OBJ(OBJ_SQLMANAGER_TEXT), ICA_MAP, map_scroller_texted, TAG_END);
		//IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_TEXT), ICA_TARGET, OBJ(OBJ_SQLMANAGER_VSCROLL), ICA_MAP, map_texted_scroller, TAG_END);

		idcmpHook.h_Entry = (HOOKFUNC) idcmpFunc;
		idcmpHook.h_Data  = window;

        if (( window = (struct Window*)IIntuition->IDoMethod(Win_Object, WM_OPEN) ))
        {
            ULONG wait;
			const char *DSNName = "test";
			char *resultValue   = NULL;
			int   result        = 0;
            struct Requester dummyRequester;
			APTR vi = NULL;
			struct Screen *_menuScreen = NULL;

			_menuScreen = IIntuition->LockPubScreen("Workbench");
			if (_menuScreen)
			{
				vi = IGadTools->GetVisualInfoA(_menuScreen, NULL);
				if (vi)
				{
					Localize();
					_menu = IGadTools->CreateMenusA(nm, NULL);
					if (_menu)
					{
						if (!IGadTools->LayoutMenus(_menu, vi, GTMN_NewLookMenus, TRUE, TAG_END))
						{
							Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_MENU),"OK", NULL);
						}
						else
						{
							IIntuition->SetMenuStrip(window, _menu);
						}
					}
					else
					{
						Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_MENU),"OK", NULL);
					}
				}
				else
				{
					Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_MENU),"OK", NULL);
				}
			}
			else
			{
				Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_LOCKWB),"OK", NULL);
			}

			/* Disable text results now. It will be enabled when table gadget will be fully efficient */
			IIntuition->OffMenu(window, FULLMENUNUM(2,3,1));

			IIntuition->UnlockPubScreen(NULL, _menuScreen);

			IIntuition->SetWindowTitles(window, winTitle, (CONST_STRPTR) -1);
            IExec->NewList(&Tables);
			IExec->NewList(&ColName);

			IIntuition->SetAttrs(GAD(OBJ_SQLMANAGER_TEXT), GA_TEXTEDITOR_VertScroller,
								 GAD(OBJ_SQLMANAGER_VSCROLL), TAG_END);

            /* Obtain the window wait signal mask. */
            IIntuition->GetAttr( WINDOW_SigMask, Win_Object, &signal );

			/* Open the Connect to DSN window */
            IIntuition->InitRequester(&dummyRequester);
            IIntuition->Request(&dummyRequester, window);
            IIntuition->SetWindowPointer(window, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);

            resultValue = (char *)OpenConnectDSNWindow();

            IIntuition->SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
            IIntuition->EndRequest(&dummyRequester, window);

            /* TEST IF WE ARE CONNECTED OR NOT */
            if (connected == 1)
            {
				/* Enable Execute Query - Disable Stop Query */
				IIntuition->OnMenu(window, FULLMENUNUM(2,0,0));
				IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));

				IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_DISCONNECT), GA_HintInfo, GetString(MSG_SB_BUTTON_DISCONNECT_DB), TAG_DONE);

                /* If YES, Refresh the GUI and try to get the DB tables */
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, FALSE, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DISCONNECT), window, NULL, GA_Selected, TRUE, TAG_DONE);

                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CONNECTION), window, NULL, BUTTON_TextPen, ~0, GA_Text, GetString(MSG_CONNECTED), TAG_DONE);
                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DSN), window, NULL, GA_Text, DSNName, TAG_DONE);
                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_USER), window, NULL, GA_Text, user, TAG_DONE);
                //IListBrowser->FreeListBrowserList(&Tables);

				FreeBrowserNodes(&Tables);
				IExec->NewList(&Tables);
				result = ListTables(OBJ(OBJ_SQLMANAGER_TABLES), &Tables, (int)3);

                IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
                if (result == 0)
                {
                    IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, &Tables, TAG_DONE);
                }
            }
            else
            {
                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CONNECTION), window, NULL, BUTTON_TextPen, 4, GA_Text, GetString(MSG_NOT_CONNECTED), TAG_DONE);
                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_USER), window, NULL, GA_Text, "", TAG_DONE);

				IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_DISCONNECT), GA_HintInfo, GetString(MSG_SB_BUTTON_CONNECT_DB), TAG_DONE);

				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, TRUE, TAG_DONE);
				IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DISCONNECT), window, NULL, GA_Selected, FALSE, TAG_DONE);
            }

			IIntuition->ActivateGadget(GAD(OBJ_SQLMANAGER_TEXT), window, NULL);

            /* Input Event Loop */
            while( !done )
            {
				uint16 code = 0;

                wait = IExec->Wait(signal|SIGBREAKF_CTRL_C);
                if (wait & SIGBREAKF_CTRL_C)
                    done = TRUE;
                else
                {
					while ((result = IIntuition->IDoMethod(Win_Object, WM_HANDLEINPUT, &code)))
                    {
						ULONG class = (result & WMHI_CLASSMASK);

						switch(class)
                        {
							case WMHI_MENUPICK:
							{
								struct MenuItem *_selectedItem = NULL;
								ULONG  itemID = 0;

								if (!_menu)
									continue;

								if (!(_selectedItem = IIntuition->ItemAddress(_menu, code)))
									continue;

								itemID = MENUNUM(code);

								while (code!= MENUNULL)
								{
									switch(MENUNUM(code))
									{
										case 0:
										{
											switch (ITEMNUM(code))
											{
												case 0: /* Create New Query file */
												{
													if (bModified == TRUE)
														TestAndExit(TRUE);
												}
												break;
												case 1: /* Load file */
												{
													BOOL nResult = TRUE;

													if (bModified == TRUE)
														nResult = TestAndExit(FALSE);
													if (nResult == TRUE)
														LoadFile();
												}
												break;
												case 2: /* Save */
													SaveFile(FALSE);
												break;
												case 3: /* Save as.. */
													SaveFile(TRUE);
												break;
												case 5: /* About */
													Requester(REQIMAGE_INFO, PROGRAM_TITLE, "SQL Manager " VERSION " (" __DATE__ ")\n\nCopyright (C) 2010 - Amigasoft.net", "OK", window);
												break;
												case 7: /* Quit */
													done = TRUE;
												break;
											}
										}
										break;
										case 1:
										break;
										case 2:
											switch (ITEMNUM(code))
											{
												case 0: /* Execute the query */
													ExecuteStopQuery();
												break;
												case 1: /* Stop the query */
													ExecuteStopQuery();
												break;
												case 3:
													switch (SUBNUM(code))
													{
														case 0:
															RESULT_TYPE = SQLMGR_RESULT_LISTBROWSER;
														break;
														case 1:
															RESULT_TYPE = SQLMGR_RESULT_TABLE;
														break;
														case 2:
															RESULT_TYPE = SQLMGR_RESULT_PLAIN_TEXT;
														break;
														case 3:
															RESULT_TYPE = SQLMGR_RESULT_XML;
														break;
													}
												break;
											}
										break;
									}
									code = _selectedItem->NextSelect;
								}
							}
							break;
                            case WMHI_GADGETUP:
                                switch (result & WMHI_GADGETMASK)
                                {
									case OBJ_SQLMANAGER_NEW:
									{
										if (bModified == TRUE)
											TestAndExit(TRUE);
									}
									break;
									case OBJ_SQLMANAGER_OPEN:
									{
										BOOL nResult = TRUE;

										if (bModified == TRUE)
											nResult = TestAndExit(FALSE);
										if (nResult == TRUE)
											LoadFile();
									}
									break;
									/* Save the current query */
									case OBJ_SQLMANAGER_SAVE:
									{
										SaveFile(FALSE);
									}
								    break;
									/* CUT text */
									case OBJ_SQLMANAGER_CUT:
									{
										IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GM_TEXTEDITOR_ARexxCmd, NULL, (ULONG) "CUT");
									}
									break;
									/* COPY text */
									case OBJ_SQLMANAGER_COPY:
									{
										IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GM_TEXTEDITOR_ARexxCmd, NULL, (ULONG) "COPY");
									}
									break;
									/* PASTE text */
									case OBJ_SQLMANAGER_PASTE:
									{
										if (!IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GM_TEXTEDITOR_ARexxCmd, NULL, (ULONG) "PASTE"))
											Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERROR_CLIPBOARD), "OK", window);
									}
									break;
									/* UNDO text */
									case OBJ_SQLMANAGER_UNDO:
									{
										IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GM_TEXTEDITOR_ARexxCmd, NULL, (ULONG) "UNDO");
									}
									break;
									/* REDO text */
									case OBJ_SQLMANAGER_REDO:
									{
										IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, GM_TEXTEDITOR_ARexxCmd, NULL, (ULONG) "REDO");
									}
									break;
									/* Execute/Stop query */
									case OBJ_SQLMANAGER_EXECQUERY:
									{
										ExecuteStopQuery();
									}
									break;
									case OBJ_SQLMANAGER_DISCONNECT:
										if (connected == 1)
										{
											if (Requester(REQIMAGE_QUESTION, PROGRAM_TITLE, GetString(MSG_DISCONNECT_FROM_DB),GetString(MSG_YESNO), window) == 1)
											{
												/* Refresh the list browser to show no rows and no columns */
												#if 0
												FreeBrowserNodes(&ColName);
												IExec->NewList(&ColName);

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_RESULTS), window, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_RESULTS), window, NULL, LISTBROWSER_Labels, &ColName, TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_RESULTS), window, NULL, LISTBROWSER_ColumnInfo, NULL, TAG_DONE);
												#endif

												/* Refresh the table list browser to show no more tables */
												FreeBrowserNodes(&Tables);
												IExec->NewList(&Tables);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
                                                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, &Tables, TAG_DONE);
												/* Change the connect/disconnect button hinthinfo */
												IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_DISCONNECT), GA_HintInfo, GetString(MSG_SB_BUTTON_CONNECT_DB), TAG_DONE);

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CONNECTION), window, NULL, BUTTON_TextPen, 4, GA_Text, GetString(MSG_NOT_CONNECTED), TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_USER), window, NULL, GA_Text, "", TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DSN), window, NULL, GA_Text, "", TAG_DONE);

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, TRUE, TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DISCONNECT), window, NULL, GA_Selected, FALSE, TAG_DONE);
												/* change the menu according the new status */
												IIntuition->OffMenu(window, FULLMENUNUM(2,0,0));
												IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));
												/* Disconnect from ODBC */
												ODBC_Disconnect();
												connected = 0;
											}
											else
											{
												IIntuition->OnMenu(window, FULLMENUNUM(2,0,0));
												IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DISCONNECT), window, NULL, GA_Selected, TRUE, TAG_DONE);
												IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_DISCONNECT), GA_HintInfo, GetString(MSG_SB_BUTTON_DISCONNECT_DB), TAG_DONE);
											}
										}
										else
										{
											/* Open the Connect to DSN window */
											IIntuition->InitRequester(&dummyRequester);
											IIntuition->Request(&dummyRequester, window);
											IIntuition->SetWindowPointer(window, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);

											resultValue = (char *)OpenConnectDSNWindow();

											IIntuition->SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
								            IIntuition->EndRequest(&dummyRequester, window);

								            /* TEST IF WE ARE CONNECTED OR NOT */
								            if (connected == 1)
								            {
								                /* If YES, Refresh the GUI and try to get the DB tables */
												IIntuition->OnMenu(window, FULLMENUNUM(2,0,0));
												IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));
												IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_DISCONNECT), GA_HintInfo, GetString(MSG_SB_BUTTON_DISCONNECT_DB), TAG_DONE);

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, FALSE, TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DISCONNECT), window, NULL, GA_Selected, TRUE, TAG_DONE);

								                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CONNECTION), window, NULL, BUTTON_TextPen, ~0, GA_Text, GetString(MSG_CONNECTED), TAG_DONE);
								                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DSN), window, NULL, GA_Text, DSNName, TAG_DONE);
								                IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_USER), window, NULL, GA_Text, user, TAG_DONE);
								                //IListBrowser->FreeListBrowserList(&Tables);

												FreeBrowserNodes(&Tables);
												IExec->NewList(&Tables);
												result = ListTables(OBJ(OBJ_SQLMANAGER_TABLES), &Tables, (int)3);

								                IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
								                if (result == 0)
								                {
								                    IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, &Tables, TAG_DONE);
								                }
								            }
								            else
								             {
												IIntuition->OffMenu(window, FULLMENUNUM(2,0,0));
												IIntuition->OffMenu(window, FULLMENUNUM(2,1,0));

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_CONNECTION), window, NULL, BUTTON_TextPen, 4, GA_Text, GetString(MSG_NOT_CONNECTED), TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_USER), window, NULL, GA_Text, "", TAG_DONE);
												IIntuition->SetAttrs(OBJ(OBJ_SQLMANAGER_DISCONNECT), GA_HintInfo, GetString(MSG_SB_BUTTON_CONNECT_DB), TAG_DONE);

												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_EXECQUERY), window, NULL, GA_Disabled, TRUE, TAG_DONE);
												IIntuition->RefreshSetGadgetAttrs(GAD(OBJ_SQLMANAGER_DISCONNECT), window, NULL, GA_Selected, FALSE, TAG_DONE);
								             }

										}
									break;
                                    case OBJ_SQLMANAGER_TABLETYPE:
                                        if (connected == 1)
                                        {
											int32 selectedNode = -1;
                                            IIntuition->GetAttrs(OBJ(OBJ_SQLMANAGER_TABLETYPE), CHOOSER_Selected, &selectedNode, TAG_DONE);
                                            if (selectedNode > -1)
                                            {
												//IListBrowser->FreeListBrowserList(&Tables);
												FreeBrowserNodes(&Tables);
												IExec->NewList(&Tables);
												result = ListTables(OBJ(OBJ_SQLMANAGER_TABLES), &Tables, selectedNode);
												if (result == 0)
												{
													IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, ~0, TAG_DONE);
                                                    IIntuition->RefreshSetGadgetAttrs((struct Gadget*)OBJ(OBJ_SQLMANAGER_TABLES), window, NULL, LISTBROWSER_Labels, &Tables, TAG_DONE);
												}
											}
                                        }
                                    break;
									case OBJ_SQLMANAGER_TABLES:
									{
										ULONG result;

										IIntuition->GetAttr(LISTBROWSER_RelEvent, OBJ(OBJ_SQLMANAGER_TABLES), (ULONG *) &result);

										switch (result)
										{
											case LBRE_DOUBLECLICK:
											 {
												struct Node *selectedNode = NULL;
												STRPTR querySelect = NULL;

												IIntuition->GetAttr(LISTBROWSER_SelectedNode, OBJ(OBJ_SQLMANAGER_TABLES), (ULONG*) &selectedNode);

												if (NULL != selectedNode)
												{
													STRPTR TABLEName;

													IListBrowser->GetListBrowserNodeAttrs(selectedNode, LBNCA_Text, &TABLEName, TAG_DONE);

													/* Allocate 14 bytes (select *..) plus the table name plus one */
													querySelect = IExec->AllocVecTags(14 + strlen(TABLEName) + 1, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
													D(bug("Allocating %d bytes (SELECT * FROM %s)\n",14 + strlen(TABLEName) + 1, TABLEName));

													if (querySelect)
													{
														snprintf(querySelect, 14 + strlen(TABLEName) + 1, "SELECT * FROM %s", TABLEName);

														IIntuition->DoGadgetMethod(GAD(OBJ_SQLMANAGER_TEXT),
																				   window,
																				   NULL,
																				   GM_TEXTEDITOR_ClearText,
																				   NULL);
														/* Put the file content into the texteditor */
														if (IIntuition->SetGadgetAttrs(GAD(OBJ_SQLMANAGER_TEXT), window, NULL,
																							GA_TEXTEDITOR_Contents, (ULONG) querySelect,
																							GA_TEXTEDITOR_Length, strlen(querySelect)))
														{
															IIntuition->RefreshGList(GAD(OBJ_SQLMANAGER_TEXT), window, NULL, 1L);
														}
														D(bug("Table:%s\n",querySelect));
														IExec->FreeVec(querySelect);
														querySelect = NULL;
													}
													else
														Requester(REQIMAGE_ERROR, PROGRAM_TITLE, GetString(MSG_ERR_OUTOFMEM),"OK", NULL);
												}
											}
											break;
										}
									}
									break;
                                }
                            break;
 							case WMHI_CLOSEWINDOW:
								if (bModified == TRUE)
								{
									done = TestAndExit(TRUE);
								}
								else
									done = TRUE;
                            break;
                        }
					}
				}
            }
        }

		/* Free memory allocated in the program */
		FreeBrowserNodes(&ColName);
		IListBrowser->FreeListBrowserList(&ColName);

		FreeBrowserNodes(&Tables);
		IListBrowser->FreeListBrowserList(&Tables);

		for (i = 0; i<10; i++)
		{
			if (images[i])
			{
				IIntuition->DisposeObject(images[i]);
				images[i] = NULL;
			}
		}
		IIntuition->DisposeObject( Win_Object );
    }

	if (TableBase)
	{
		IIntuition->CloseClass((struct ClassLibrary *) TableBase);
	}
	if (filename)
	{
		IExec->FreeVec(filename);
		filename = NULL;
	}

    if (connected==1) ODBC_Disconnect();
	if (locale)
	{
		ILocale->CloseLocale( locale );
		locale = NULL;
	}
	if (catalog)
	{
		ILocale->CloseCatalog( catalog );
		catalog = NULL;
	}

	if (SQLManagerPort)
	{
		IExec->FreeSysObject(ASOT_PORT, SQLManagerPort);
		SQLManagerPort = 0;
	}

    return SQLMGR_ERROR_NOERROR;
}
