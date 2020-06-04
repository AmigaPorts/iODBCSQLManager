#ifndef __SQLMANAGER_UTILS_H__
#define __SQLMANAGER_UTILS_H__

#include <proto/exec.h>
#include <proto/chooser.h>
#include <proto/listbrowser.h>
#include <proto/intuition.h>

void FreeChooserLabels( struct List *chooserlist );
void FreeBrowserNodes( struct List *browserlist );
LONG ReadTEFile(struct Gadget *TE, struct Window *win, CONST_STRPTR filename);
LONG SaveTEFile(struct Gadget *TE, struct Window *win, CONST_STRPTR filename);
BOOL FileExists(CONST_STRPTR filename);
void SleepWindow(struct Window *tempWindow, BOOL state);
STRPTR ChooseFilename(CONST_STRPTR fileext);
STRPTR Replace(STRPTR st, CONST_STRPTR orig, CONST_STRPTR repl);
STRPTR ParseXML(STRPTR st);

#endif

