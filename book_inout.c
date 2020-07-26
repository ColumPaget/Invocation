#include "book_inout.h"


/* These functions relate to the invocation book in/out system that can send a file to a client app and/or
read one back into place from the client app. This means clients can be given access to files normally owned
by root (e.g. crontabs) without the user and their file editor program ever having root permissions
*/

void BookoutFileToClient(STREAM *Client, const char *Path, ListNode *Vars)
{
char *Tempstr=NULL, *Msg=NULL;
struct stat Stat;
int result;
STREAM *S;

Tempstr=SubstituteVarsInString(Tempstr, Path, Vars, 0);
result=stat(Tempstr, &Stat);

if ((result==0) && (S_ISREG(Stat.st_mode)))
{
	S=STREAMOpen(Tempstr, "r");
	if (S)
	{
		Msg=FormatStr(Msg, "bookout %llu %s\n", (unsigned long long) Stat.st_size, GetBasename(Tempstr));
		STREAMWriteLine(Msg, Client);
		STREAMSendFile(S, Client, 0, SENDFILE_KERNEL | SENDFILE_LOOP);
		STREAMClose(S);
	}
}

Destroy(Tempstr);
Destroy(Msg);
}


void BookinFileFromClient(STREAM *Client, const char *iPath, ListNode *Vars)
{
char *Tempstr=NULL, *Path=NULL, *Msg=NULL;
size_t Size;
struct stat Stat;
const char *ptr;
int result;
STREAM *S;

Path=SubstituteVarsInString(Path, iPath, Vars, 0);
Msg=FormatStr(Msg, "bookin %s\n", GetBasename(Path));
Tempstr=MCopyStr(Tempstr, Path, "+", NULL);
S=STREAMOpen(Tempstr, "w");
if (S)
{
	STREAMWriteLine(Msg, Client);
	STREAMFlush(Client);
	Msg=STREAMReadLine(Msg, Client);
	ptr=GetToken(Msg, " ", &Tempstr,0);
	if (strcmp(Tempstr, "bookin")==0)
	{
	Size=(size_t) strtoll(ptr, NULL, 10);
	if (Size > 0) STREAMSendFile(Client, S, Size, SENDFILE_LOOP);
	Tempstr=MCopyStr(Tempstr, Path, "+", NULL);
	stat(Path, &Stat);
	rename(Tempstr, Path); 
	chmod(Path, Stat.st_mode);
	chown(Path, Stat.st_uid, Stat.st_gid);
	}
	else 
	{
	Tempstr=MCopyStr(Tempstr, Path, "+", NULL);
	unlink(Tempstr);
	}
	STREAMClose(S);
}

Destroy(Tempstr);
Destroy(Path);
Destroy(Msg);
}


