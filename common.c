#include "common.h"

int IsItemInList(const char *Item, const char *StrList)
{
    int result=FALSE;
    char *Name=NULL;
    const char *ptr;

    if (strcmp(Item, "*")==0) return(TRUE);

    ptr=GetToken(StrList, "\\S|,", &Name, GETTOKEN_MULTI_SEP);
    while (ptr)
    {
        if (strcmp(Item, Name) ==0) result=TRUE;
        ptr=GetToken(ptr, "\\S|,", &Name, GETTOKEN_MULTI_SEP);
    }

    Destroy(Name);

    return(result);
}
