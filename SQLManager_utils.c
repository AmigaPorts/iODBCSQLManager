/******************************************************************/
/* Support utilities file for SQL Manager                         */
/* $VER: SQLManager_utils.c 1.0 (02.07.2010) ©2010 Andrea Palmatè */
/******************************************************************/

#include <proto/exec.h>
#include <proto/chooser.h>
#include <proto/listbrowser.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/intuition.h>
#include <proto/texteditor.h>
#include <gadgets/texteditor.h>
#include <classes/requester.h>

#include <string.h>

#include "SQLManager.h"
#include "SQLManager_utils.h"
#include "SQLManager_glue.h"
#include "SQLManager_cat.h"

#include "debug.h"

extern char winTitle[255];

/******************************************************************************/
/*                                                                            */
/* This function remove all nodes from a chooser list						  */
/*                                                                            */
/* Parameters:                                                                */
/*      struct List *chooserlist     - Pointer to the list to be freed        */
/*                                                                            */
/* Return Value:                                                              */
/*      NONE                                               					  */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void FreeChooserLabels( struct List *chooserlist )
{

	if (chooserlist)
	{
		struct Node *node;

		while(( node = IExec->RemHead( chooserlist )))
		{
			IChooser->FreeChooserNode( node );
			node = NULL;
		}
		chooserlist = NULL;
	}
}

/******************************************************************************/
/*                                                                            */
/* This function remove all allocated nodes from a ListBrowser				  */
/*                                                                            */
/* Parameters:                                                                */
/*      struct List *browserlist     - Pointer to the list to be freed 		  */
/*                                                                            */
/* Return Value:                                                              */
/*      NONE                                               					  */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void FreeBrowserNodes( struct List *browserlist )
{

	if (browserlist)
	{
		struct Node *node;

		while(( node = IExec->RemHead( browserlist )))
		{
			IListBrowser->FreeListBrowserNode( node );
			node = NULL;
		}

		IListBrowser->FreeListBrowserList( browserlist );
		browserlist = NULL;
	}
}

/******************************************************************************/
/*                                                                            */
/* This function Sleep/Wake a window 										  */
/*                                                                            */
/* Parameters:                                                                */
/*      struct Window  *tempWindow     - Pointer to the window to sleep/awake */
/*	 	BOOL			state		   - the new state of the window		  */
/*				TRUE	- Put the window asleep								  */
/*				FALSE	- Put the window awake								  */
/*                                                                            */
/* Return Value:                                                              */
/*      NONE                                               					  */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void SleepWindow(struct Window *tempWindow, BOOL state)
{
	struct Requester dummyRequester;

	if (state == TRUE)
	{
		IIntuition->InitRequester(&dummyRequester);
		IIntuition->Request(&dummyRequester, tempWindow);
		IIntuition->SetWindowPointer(tempWindow, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_DONE);
	}
	else
	{
		IIntuition->SetWindowPointer(tempWindow, WA_BusyPointer, FALSE, TAG_DONE);
		IIntuition->EndRequest(&dummyRequester, tempWindow);
	}
}

/******************************************************************************/
/*                                                                            */
/* This function test if a file exists 										  */
/*                                                                            */
/* Parameters:                                                                */
/*      CONST_STRPTR   filename     - The file to check						  */
/*                                                                            */
/* Return Value:                                                              */
/*      TRUE if the file exists                                               */
/*      FALSE if not					                                      */
/*                                                                            */
/******************************************************************************/
BOOL FileExists(CONST_STRPTR filename)
{
	BPTR f;
	f = IDOS->Lock(filename, SHARED_LOCK);
	if (f)
	{
		IDOS->UnLock(f);
		return TRUE;
	}
	else
		return FALSE;
}

/******************************************************************************/
/*                                                                            */
/* This function load a file from the disk and show it in a texteditor.gadget */
/*                                                                            */
/* Parameters:                                                                */
/*      struct Gadget *TE           - The texteditor.gadget                   */
/*      struct Window *win          - The window where the gadget is          */
/*      CONST_STRPTR   filename     - The file to load into the texteditor    */
/*                                                                            */
/* Return Value:                                                              */
/*      0 if success                                                          */
/*      a non 0 value if an error occurs                                      */
/*                                                                            */
/******************************************************************************/
LONG ReadTEFile(struct Gadget *TE, struct Window *win, CONST_STRPTR filename)
{
	BPTR f;
	LONG  filesize;
	ULONG len;
	char *buffer;
	LONG error = 0;
	struct ExamineData *edata = NULL;

	/* Open the file */
	f = IDOS->Open(filename, MODE_OLDFILE);
	if (f)
	{
		/* Get the file size */
		if (( edata = IDOS->ExamineObjectTags(EX_FileHandle, f, TAG_DONE) ))
		{
			/* if the file is less than 2GB.. */
			if (edata->FileSize < 0x7ffffffeLL)
			{
				filesize = (LONG) (0xFFFFFFFFLL & edata->FileSize);
				D(bug("Filesize: %ld\n", filesize));

				/* Allocate the memory for the file */
				buffer = IExec->AllocVecTags(filesize + 1,
											 AVT_Type, MEMF_SHARED,
											 TAG_DONE);

				if (buffer)
				{
					/* Read the file */
					len = IDOS->Read(f, buffer, filesize);
					D(bug("len=%ld\n",len));
					if (len!=filesize)
					{
						error = IDOS->IoErr();
						D(bug("error:%ld\n",error));
					}

				    /*
					if (len>filesize);
					{
						len = 0L;

						D(bug("error: len=%ld\n",len));
					}
					*/
					buffer[len] = '\0';

					/* Put the file content into the texteditor */
					if (IIntuition->SetGadgetAttrs(TE, win, NULL,
														GA_TEXTEDITOR_Contents, (ULONG) buffer,
														GA_TEXTEDITOR_Length, len))
					{
						IIntuition->RefreshGList(TE, win, NULL, 1L);
					}

					IExec->FreeVec(buffer); /* Free the buffer */
					buffer = NULL;

					/* Let's go to the first texteditor character */
					IIntuition->SetGadgetAttrs(TE, win, NULL, GA_TEXTEDITOR_CursorX, 0, GA_TEXTEDITOR_CursorY, 0, TAG_END);
				}
				else
				{
					error = SQLMGR_ERROR_NOMEM;
					D(bug("Cannot allocate memory for the file!\n"));
				}
			}
			else
			{
				error = SQLMGR_ERROR_FILETOOLARGE;
				D(bug("File is greater than 2GB!\n"));
			}
			IDOS->FreeDosObject(DOS_EXAMINEDATA, edata);
		}
		else
			D(bug("File is empty!\n"));

		IDOS->Close(f);
	}
	return error;
}

/******************************************************************************/
/*                                                                            */
/* This function replace all occurence of a pattern in a string				  */
/*                                                                            */
/* Parameters:                                                                */
/*      STRPTR st           	- The original string		                  */
/*      CONST_STRPTR orig       - The pattern to change          			  */
/*      CONST_STRPTR repl     	- The new pattern 							  */
/*                                                                            */
/* Return Value:                                                              */
/*      the new string														  */
/*                                                                            */
/******************************************************************************/
STRPTR Replace(STRPTR st, CONST_STRPTR orig, CONST_STRPTR repl)
{
	char *ret, *sr;
	size_t i, count = 0;
	size_t newlen = strlen(repl);
	size_t oldlen = strlen(orig);

	if (newlen != oldlen)
	{
		for (i = 0; st[i] != '\0'; )
		{
			if (memcmp(&st[i], orig, oldlen) == 0)
				count++, i += oldlen;
			else
				i++;
		}
	}
	else
		i = strlen(st);

	ret = malloc(i + 1 + count * (newlen - oldlen));
	if (ret == NULL)
		return NULL;

	sr = ret;
	while (*st)
	{
		if (memcmp(st, orig, oldlen) == 0)
		{
			memcpy(sr, repl, newlen);
			sr += newlen;
			st += oldlen;
		}
		else
			*sr++ = *st++;
	}
	*sr = '\0';

	return ret;
}

/******************************************************************************/
/*                                                                            */
/* This function replace some patter in a XML string						  */
/*                                                                            */
/* Parameters:                                                                */
/*      STRPTR st		            - The string to be parsed				  */
/*                                                                            */
/* Return Value:                                                              */
/*      The new string														  */
/*                                                                            */
/******************************************************************************/
STRPTR ParseXML(STRPTR st)
{
	st = Replace(st,"&","&amp;");
	st = Replace(st,"<","&lt;");
	st = Replace(st,">","&gt;");
	return strdup(st);
}

/******************************************************************************/
/*                                                                            */
/* This function save a file to disk										  */
/*                                                                            */
/* Parameters:                                                                */
/*      struct Gadget *TE           - The texteditor.gadget                   */
/*      struct Window *win          - The window where the gadget is          */
/*      CONST_STRPTR   filename     - The filename of the file being saved    */
/*                                                                            */
/* Return Value:                                                              */
/*      0 if success                                                          */
/*      a non 0 value if an error occurs                                      */
/*                                                                            */
/******************************************************************************/
LONG SaveTEFile(struct Gadget *TE, struct Window *win, CONST_STRPTR filename)
{
	BPTR f;
	ULONG error = 0;

	/* Open the file */
	f = IDOS->Open(filename, MODE_NEWFILE);
	if (f)
	{
		STRPTR text = (STRPTR) IIntuition->DoGadgetMethod( TE, win, NULL, GM_TEXTEDITOR_ExportText, NULL);
		if (text)
		{
			ULONG bytes = strlen(text);
			if (IDOS->Write(f, text, bytes)!=bytes)
			{
				error = IDOS->IoErr();
			}
			IExec->FreeVec(text);
			text = NULL;
		}
		else
			error = -1;

		IDOS->Close(f);
	}
	else
		error = IDOS->IoErr();

	return error;
}

/******************************************************************************/
/*                                                                            */
/* This function open an ASL requester and returns a file					  */
/*                                                                            */
/* Parameters:                                                                */
/*      CONST_STRPTR fileext        - The ASL file pattern                    */
/*                                                                            */
/* Return Value:                                                              */
/*      The file name if the user click on Open 						      */
/*      NULL if the user click on Cancel									  */
/*                                                                            */
/******************************************************************************/
STRPTR ChooseFilename(CONST_STRPTR fileext)
{
	struct FileRequester *FR = NULL;
	char *newfile = NULL;

	FR = IAsl->AllocAslRequest(ASL_FileRequest, NULL);
	if (FR)
	{
		if ( (IAsl->AslRequestTags(FR,
									ASLFR_TitleText, 	  (ULONG) GetString(MSG_RESULTS_FILENAME),
									ASLFR_RejectIcons,    TRUE,
									ASLFR_DoPatterns,     TRUE,
									ASLFR_SleepWindow,    TRUE,
									ASLFR_InitialPattern, (ULONG) fileext,
									ASLFR_DoSaveMode,     TRUE,
									TAG_DONE) ) == FALSE)
		{
			D(bug("User has click on Cancel\n"));
		}
		else
		{
			newfile = IExec->AllocVecTags(strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE);
			if (newfile)
			{
				D(bug("file:%s%s\n",FR->fr_Drawer,FR->fr_File));
				snprintf(newfile, strlen(FR->fr_Drawer) + strlen(FR->fr_File) + 1, "%s%s",FR->fr_Drawer, FR->fr_File);
				if (FileExists(newfile) == TRUE)
				{
					if (Requester(REQIMAGE_QUESTION, PROGRAM_TITLE, GetString(MSG_ASL_FILEEXISTS), GetString(MSG_YESNO), NULL) == 1)
					{
						return newfile;
					}
				}
				else
					return newfile;
			}
		}
	}
	return NULL;
}
