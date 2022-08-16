#include "appdb.h"

#define APPDB_PATH "/var/db/invocation/clients.db"

TGrant *AppDBParseEntry(const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    TGrant *Grant;

    if (StrValid(Config) && (*Config != '#'))
    {
        Grant=(TGrant *) calloc(1, sizeof(TGrant));
        ptr=GetNameValuePair(Config, " ", "=", &Name, &Value);
        while (ptr)
        {
            if (strcmp(Name,"grant")==0) Grant->Type=CopyStr(Grant->Type, Value);
            if (strcmp(Name,"user")==0) Grant->User=CopyStr(Grant->User, Value);
            if (strcmp(Name,"group")==0) Grant->Group=CopyStr(Grant->Group, Value);
            if (strcmp(Name,"program")==0) Grant->Program=CopyStr(Grant->Program, Value);
            if (strcmp(Name,"hash")==0) Grant->Hash=CopyStr(Grant->Hash, Value);
            if (strcmp(Name,"size")==0) Grant->Size=(size_t) strtoll(Value,NULL,10);
            ptr=GetNameValuePair(ptr, " ", "=", &Name, &Value);
        }
    }

    DestroyString(Name);
    DestroyString(Value);

    return(Grant);
}


void AppDBDestroyEntry(void *p_Entry)
{
    TGrant *Entry;

    Entry=(TGrant *) p_Entry;
    if (Entry)
    {
        Destroy(Entry->Type);
        Destroy(Entry->User);
        Destroy(Entry->Group);
        Destroy(Entry->Program);
        Destroy(Entry->Hash);
        free(Entry);
    }
}



int AppDBCheckEntry(TGrant *Entry, const char *Grant, const char *AppPath, struct stat *Stat)
{
    int result=FALSE;
    char *Tempstr=NULL;

    if (! Entry) return(FALSE);
    if (! StrValid(Grant)) return(FALSE);
    if (! StrValid(AppPath)) return(FALSE);

    if (! IsItemInList(Grant, Entry->Type)) return(FALSE);


    if ( StrValid(Entry->Program) && (strcmp(Entry->Program, AppPath) !=0) ) return(FALSE);


    //we can reuse Tempstr now
    HashFile(&Tempstr, "sha256", AppPath, ENCODE_BASE64);

    if ((Stat->st_size == Entry->Size) && (strcmp(Tempstr, Entry->Hash)==0) ) result=TRUE;

    Destroy(Tempstr);

    return(result);
}



int AppDBCheck(const char *Grant, const char *AppPath)
{
    char *Tempstr=NULL;
    int result=FALSE;
    TGrant *Entry=NULL;
    struct stat Stat;
    STREAM *S;

    if (stat(AppPath, &Stat)==-1)
    {
        Tempstr=MCopyStr("error: Failed to open '",AppPath,"' - ",NULL);
        perror(Tempstr);
        Destroy(Tempstr);
        return(FALSE);
    }

    MakeDirPath(APPDB_PATH, 0700);
    S=STREAMOpen(APPDB_PATH, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            StripLeadingWhitespace(Tempstr);
            Entry=AppDBParseEntry(Tempstr);
            if (AppDBCheckEntry(Entry, Grant, AppPath, &Stat)) result=TRUE;
            AppDBDestroyEntry(Entry);

            if (result) break;

            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }


    Destroy(Tempstr);

    return(result);
}



void AppDBAdd(const char *Grant, const char *User, const char *Group, const char *AppPath)
{
    char *Tempstr=NULL, *Name=NULL, *Hash=NULL;
    const char *ptr;
    STREAM *S;
    ListNode *Items=NULL, *Curr=NULL;
    struct stat Stat;

    if (stat(AppPath, &Stat)==-1)
    {
        Tempstr=MCopyStr(Tempstr, "error: Failed to open '",AppPath,"' - ",NULL);
        perror(Tempstr);
        Destroy(Tempstr);
        return;
    }

    Items=ListCreate();

    MakeDirPath(APPDB_PATH, 0700);
    S=STREAMOpen(APPDB_PATH, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            GetToken(Tempstr, "\\S",&Name,GETTOKEN_QUOTES);
            SetVar(Items, Name, Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    HashFile(&Hash, "sha256", AppPath, ENCODE_BASE64);
    Tempstr=FormatStr(Tempstr,"grant='%s' program='%s' size=%llu hash=%s", Grant, AppPath, (unsigned long long) Stat.st_size, Hash);

    if (StrValid(User)) Tempstr=MCatStr(Tempstr, " user=", User, NULL);
    if (StrValid(Group)) Tempstr=MCatStr(Tempstr, " group=", Group, NULL);
    SetVar(Items, AppPath, Tempstr);

    Tempstr=MCopyStr(Tempstr, APPDB_PATH, "+", NULL);
    S=STREAMOpen(Tempstr, "w");
    if (S)
    {
        Curr=ListGetNext(Items);
        while (Curr)
        {
            STREAMWriteString(Curr->Item, S);
            STREAMWriteString("\n", S);
            Curr=ListGetNext(Curr);
        }
        STREAMClose(S);
    }

    rename(Tempstr, APPDB_PATH);

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Hash);
}
