#include "common.h"

void ClientBookoutFile(STREAM *Src, const char *Details)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr;
STREAM *Dest=NULL;
size_t Size;

ptr=GetToken(Details, " ", &Token, 0);
Size=(size_t) strtoll(Token,NULL,10);
Token=CopyStr(Token, ptr);
StripTrailingWhitespace(Token);
Dest=STREAMOpen(Token, "w");
if (Dest)
{
	if (Size > 0) STREAMSendFile(Src, Dest, Size, SENDFILE_LOOP);
	STREAMClose(Dest);
}

DestroyString(Tempstr);
DestroyString(Token);
}

void ClientBookinFile(STREAM *Dest, const char *Details)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr;
struct stat Stat;
STREAM *Src=NULL;
size_t Size;

Token=CopyStr(Token, Details);
StripTrailingWhitespace(Token);
Src=STREAMOpen(Token, "r");
if (Src)
{
	Tempstr=FormatStr(Tempstr, "bookin %d %s\n",Src->Size,Token);
	STREAMWriteLine(Tempstr, Dest);
	if (Size > 0) STREAMSendFile(Src, Dest, 0, SENDFILE_LOOP);
	STREAMFlush(Dest);
	STREAMClose(Src);
}
else Tempstr=FormatStr(Tempstr, "error can't open %s\n",Token);

DestroyString(Tempstr);
DestroyString(Token);
}



void ClientRun(const char *Details)
{
char *Tempstr=NULL;

Tempstr=CopyStr(Tempstr, Details);
StripTrailingWhitespace(Tempstr);
system(Tempstr);
DestroyString(Tempstr);
}


void ParseCommandLineArgs(int argc, char *argv[], char **Cmd, char **Vars)
{
CMDLINE *CL;
char *Args=NULL;
const char *ptr=NULL;

CL=CommandLineParserCreate(argc, argv);
ptr=CommandLineNext(CL);
*Cmd=CopyStr(*Cmd, ptr);
*Vars=CopyStr(*Vars, "");
ptr=CommandLineNext(CL);

while (ptr)
{
	if ((*ptr=='-') || (! strchr(ptr, '='))) Args=MCatStr(Args, ptr, " ",NULL);
	else *Vars=MCatStr(*Vars, ptr, " ",NULL);
	ptr=CommandLineNext(CL);
}

if (StrValid(Args)) *Vars=MCatStr(*Vars, " args='", Args, "'", NULL);

DestroyString(Args);
}



void ClientAuth(STREAM *InvokeS)
{
STREAM *S;
char *Tempstr=NULL, *Msg=NULL;

S=STREAMFromFD(0);
printf("Authentication Required\n"); fflush(NULL);
Tempstr=STREAMReadLine(Tempstr, S);
StripTrailingWhitespace(Tempstr);
Msg=MCopyStr(Msg, "AUTH password=", Tempstr, "\n", NULL);
STREAMWriteLine(Msg, InvokeS);
STREAMClose(S);

Destroy(Tempstr);
Destroy(Msg);
}


int main(int argc, char *argv[])
{
char *Tempstr=NULL, *Token=NULL, *Cmd=NULL, *Args=NULL;
STREAM *S, *StdOut;
const char *ptr;
int i;

ParseCommandLineArgs(argc, argv, &Cmd, &Args);

StdOut=STREAMFromFD(1);
S=STREAMOpen("unix:/dev/invocation","rw");
if (S)
{
	Tempstr=MCopyStr(Tempstr, "invoke: ", Cmd, " ", Args, "\n",NULL);
	STREAMWriteLine(Tempstr, S);
	STREAMFlush(S);

	Tempstr=STREAMReadLine(Tempstr, S);
	while (Tempstr)
	{
	  StripTrailingWhitespace(Tempstr);
		ptr=GetToken(Tempstr, " ",&Token,0);
		if (strcasecmp(Token, "bookout")==0) ClientBookoutFile(S, ptr);
		else if (strcasecmp(Token, "bookin")==0) ClientBookinFile(S, ptr);
		else if (strcasecmp(Token, "clientrun")==0) ClientRun(ptr);
		else if (strcasecmp(Token, "auth")==0) ClientAuth(S);
		else if (strcasecmp(Token, "okay:")==0) STREAMSendFile(S, StdOut, 0, SENDFILE_LOOP);
		else printf("%s",Tempstr);
		Tempstr=STREAMReadLine(Tempstr, S);
	}
	STREAMClose(S);
}
else printf("FAIL: could not connect\n");

DestroyString(Tempstr);
DestroyString(Token);

return(0);
}
