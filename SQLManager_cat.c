/**********************************************************
Catalog description file for SQL Manager
$VER: SQLManager.cd 1.0 (02.07.2010) ©2010 Andrea Palmatè
***********************************************************/

#include	<sys/types.h>
#include	<proto/locale.h>
#include	<proto/intuition.h>
#include	<intuition/intuition.h>
#include 	<stdio.h>

#define	CATCOMP_ARRAY

#include	"SQLManager_cat.h"
extern struct Catalog	*catalog;

char *GetString( LONG id )
{
	char *str = "";
	int i;

	for ( i = 0 ; i < 150 ; i++ )
	{
		if ( CatCompArray[i].cca_ID == id )
		{
			str = (char *)CatCompArray[i].cca_Str;
			break;
		}
	}
	return( (char *)ILocale->GetCatalogStr( catalog, id, str ) );
}
