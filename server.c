#include "common.h"
#include "peer.h"
#include "appdb.h"
#include "auth.h"
#include "book_inout.h"
#include <wait.h>

#define FLAG_NODEMON 1

ListNode *Config=NULL;
int Flags=0;

static int HandleClientRequest(STREAM *S, TPeerInfo *Peer, const char *AuthTypes);



static void LoadConfig(const char *Path)
{
STREAM *S;
char *Tempstr=NULL;

S=STREAMOpen(Path, "r");
if (S)
{
Tempstr=STREAMReadDocument(Tempstr, S);

if (Config) ParserItemsDestroy(Config);
Config=ParserParseDocument("config", Tempstr);
STREAMClose(S);
}

Destroy(Tempstr);
}


static int CheckAuth(const char *AuthTypes, STREAM *S, TPeerInfo *Peer)
{
if (Peer->Flags & PEER_AUTHENTICATED) return(TRUE);
STREAMWriteLine("AUTH\n", S);
STREAMFlush(S);
while (HandleClientRequest(S, Peer, AuthTypes));

if (Peer->Flags & PEER_AUTHENTICATED) return(TRUE);
return(FALSE);
}




int CheckPermission(STREAM *PeerS, TPeerInfo *Peer, const char *Command, const char *Args, ListNode *Rules)
{
ListNode *Curr;
int result=FALSE;

syslog(LOG_DEBUG, "checking rules for peer %s@%s", Peer->UserName, Peer->ExePath);
//if a grant exists for the program, then that's good enough 
if (AppDBCheck(Command, Peer->ExePath)) return(TRUE);

// if any permission exists, and fails, return FALSE. 
// If none exist then 'result' will be false, and that will be returned.

Curr=ListGetNext(Rules);
while (Curr)
{
	syslog(LOG_DEBUG, "rule '%s %s' peer %s@%s", Curr->Tag, (const char *) Curr->Item, Peer->UserName, Peer->ExePath);
	if (strcasecmp(Curr->Tag, "user")==0) 
	{
		if (IsItemInList(Peer->UserName, (const char *) Curr->Item)) result=TRUE;
		else 
		{
			syslog(LOG_DEBUG, "rule fail: '%s %s' peer %s@%s", Curr->Tag, (const char *) Curr->Item, Peer->UserName, Peer->ExePath);
			return(FALSE);
		}
	}

	if (strcasecmp(Curr->Tag, "group")==0)
	{
		if (IsItemInList(Peer->GroupName, (const char *) Curr->Item)) result=TRUE;
		else 
		{
			syslog(LOG_DEBUG, "rule fail: '%s %s' peer %s@%s", Curr->Tag, (const char *) Curr->Item, Peer->UserName, Peer->ExePath);
			return(FALSE);
		}
	}

	if (strcasecmp(Curr->Tag, "program")==0)
	{
		if (IsItemInList(Peer->ExePath, (const char *) Curr->Item)) result=TRUE;
		else
		{
			syslog(LOG_DEBUG, "rule fail: '%s %s' peer %s@%s", Curr->Tag, (const char *) Curr->Item, Peer->UserName, Peer->ExePath);
			return(FALSE);
		}
	}


	if (strcasecmp(Curr->Tag, "auth")==0) 
	{
		if (CheckAuth(Curr->Item, PeerS, Peer)) result=TRUE;
		else 
		{
			syslog(LOG_DEBUG, "rule fail: '%s %s' peer %s@%s", Curr->Tag, (const char *) Curr->Item, Peer->UserName, Peer->ExePath);
			return(FALSE);
		}
	}

	Curr=ListGetNext(Curr);
}


return(result);
}


//search through list of configs, looking for one that relates to this peer
int FindPermission(STREAM *S, TPeerInfo *Peer, const char *Command, const char *Args)
{
ListNode *Curr;

syslog(LOG_DEBUG, "Find permission for peer %s@%s", Peer->UserName, Peer->ExePath);

Curr=ListGetNext(Config);
while (Curr)
{
	if (strcmp(Curr->Tag, Command)==0) return(CheckPermission(S, Peer, Command, Args, (ListNode *) Curr->Item));
	if (strcmp(Curr->Tag, Peer->ExePath)==0) return(CheckPermission(S, Peer, Command, Args, (ListNode *) Curr->Item));
	if (strcmp(Curr->Tag, GetBasename(Peer->ExePath))==0) return(CheckPermission(S, Peer, Command, Args, (ListNode *) Curr->Item));
	Curr=ListGetNext(Curr);
}

return(FALSE);
}


static int HandleClientAuth(STREAM *S, TPeerInfo *Peer, const char *Details, const char *AuthTypes)
{
char *Password=NULL, *Name=NULL, *Value=NULL;
const char *ptr;

ptr=GetNameValuePair(Details, " ", "=", &Name, &Value);
while (ptr)
{
	if (strcmp(Name, "password")==0) Password=CopyStr(Password, Value); 
	ptr=GetNameValuePair(ptr, " ", "= ", &Name, &Value);
}


//if authentication types were specified then use that, else default to 'login' type
if (StrValid(AuthTypes)) Value=CopyStr(Value, AuthTypes);
else Value=CopyStr(Value, "login");

if (AuthPAM(Value, Peer->UserName, Password))
{
	Peer->Flags |= PEER_AUTHENTICATED;
	STREAMWriteLine("OKAY: Authenticated\n", S);
}
else STREAMWriteLine("FAIL: Authentication failed\n", S);

//AuthPAMCheckAccount(const char *UserName);
AuthPAMClose();


Destroy(Password);
Destroy(Name);
Destroy(Value);

return(Peer->Flags & PEER_AUTHENTICATED);
}






int ProcessExpect(STREAM *S, const char *ExpectName, ListNode *Vars, ListNode *Config, int Timeout)
{
ListNode *Dialog, *Curr;
char *Token=NULL, *Expect=NULL, *Reply=NULL;
const char *ptr;
int result=FALSE;

Dialog=ListCreate();
Curr=ParserOpenItem(Config, ExpectName);
while (Curr)
{
if (Curr->ItemType==ITEM_VALUE)
{
	ptr=GetToken(Curr->Item, "\\S", &Token, GETTOKEN_QUOTES);
	Expect=SubstituteVarsInString(Expect, Token, Vars, 0);
	ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
	Reply=SubstituteVarsInString(Reply, Token, Vars, 0);

	Reply=CatStr(Reply,"\n");
	if (strcmp(Curr->Tag,"expect")==0) ExpectDialogAdd(Dialog, Expect, Reply, 0);
	else if (strcmp(Curr->Tag,"optional")==0) ExpectDialogAdd(Dialog, Expect, Reply, DIALOG_OPTIONAL);
	else if (strcmp(Curr->Tag,"fail")==0) ExpectDialogAdd(Dialog, Expect, Reply, DIALOG_OPTIONAL | DIALOG_FAIL);
	else if (strcmp(Curr->Tag,"end")==0) ExpectDialogAdd(Dialog, Expect, Reply, DIALOG_END);
}

Curr=ListGetNext(Curr);
}

result=STREAMExpectDialog(S, Dialog, Timeout);
ExpectDialogDestroy(Dialog);

Destroy(Expect);
Destroy(Token);
Destroy(Reply);

return(result);
}



//buffer overflows etc are often triggered by long command-line
//arguments. Thus we can check for long arguments being requested
static int InvokeCheckArgs(const char *Cmd, int MaxLen)
{
int RetVal=TRUE;
char *Token=NULL;
const char *ptr;

ptr=GetToken(Cmd, "\\S", &Token, 0);
while (ptr)
{
if (StrLen(Token) > MaxLen)
{
RetVal=FALSE;
break;
}
ptr=GetToken(ptr, "\\S", &Token, 0);
}

Destroy(Token);

return(RetVal);
}



static void RunCommand(STREAM *PeerS, const char *Details, ListNode *Vars, ListNode *Config)
{
STREAM *CmdS;
char *Cmd=NULL, *Tempstr=NULL, *Expect=NULL;
char *Name=NULL, *Value=NULL;
const char *ptr;
int result, Timeout=0, maxlen=0;

ptr=GetToken(Details, "\\S", &Tempstr, GETTOKEN_QUOTES);
Cmd=SubstituteVarsInString(Cmd, Tempstr, Vars, SUBS_SHELL_SAFE);

//some absolute last checks on the command we are going to run
maxlen=atoi(GetVar(Vars, "InvokeMaxArgLen"));
if ( (maxlen > 0) && (! InvokeCheckArgs(Cmd, maxlen)) )
{
syslog(LOG_WARNING, "peer requested command with overlong args: %s", Cmd);
Destroy(Tempstr);
Destroy(Cmd);
return;
}


//we will now use tempstr to build the arguments to the Spawn command
Tempstr=CopyStr(Tempstr, "");
ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
while (ptr)
{
if (strcmp(Name,"expect")==0) Expect=CopyStr(Expect, Value);
else if (strcmp(Name,"pty")==0) Tempstr=CatStr(Tempstr, "pty ");
else if (strcmp(Name,"timeout")==0) Timeout=atoi(Value);
else if (strcmp(Name,"capabilities")==0) Tempstr=MCatStr(Tempstr, Value);
else if (strcmp(Name,"caps")==0) Tempstr=MCatStr(Tempstr, "capabilities='", Value,"' ", NULL);
else Tempstr=MCatStr(Tempstr,"",Name,"='",Value,"' ",NULL);
ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
}

fprintf(stderr, "RUN: %s caps=[%s] expect=[%s] Timeout=%d\n", Cmd, Tempstr, Expect, Timeout);
CmdS=STREAMSpawnCommand(Cmd, Tempstr);
if (CmdS)
{
	if (StrValid(Expect)) result=ProcessExpect(CmdS, Expect, Vars, Config, Timeout);
	else result=TRUE;

	if (result)
	{
	STREAMWriteLine("OKAY: Command output follows\n", PeerS);
	STREAMSendFile(CmdS, PeerS, 0, SENDFILE_LOOP | SENDFILE_FLUSH);
	}
	else STREAMWriteLine("FAIL: convo failed\n", PeerS);

	STREAMClose(CmdS);
}
else STREAMWriteLine("FAIL: No such command\n", PeerS);

Destroy(Tempstr);
Destroy(Expect);
Destroy(Name);
Destroy(Value);
Destroy(Cmd);
}


static int CheckRequirement(const char *Details, STREAM *S, TPeerInfo *Peer, ListNode *Vars)
{
char *Type=NULL;
const char *ptr;
int result=TRUE;
struct stat FStat;

ptr=GetToken(Details, "\\S", &Type, 0);
switch (*Type)
{
	case 'a':
	case 'A':
	//file must be absent
	if ((strcasecmp(Type, "absent")==0) && (access(ptr, F_OK) ==0)) result=FALSE;

	//no command-line-argument can be longer than arglen
	if (strcasecmp(Type, "arglen")==0) SetVar(Vars, "InvokeMaxArgLen", ptr);
	break;

	case 'd':
	case 'D':
	//directory must exist
	if (strcasecmp(Type, "directory")==0) 
	{
	if (
			(stat(ptr, &FStat) != 0) || 
			(! S_ISDIR(FStat.st_mode))
	) result=FALSE;
	}
	break;

	case 'e':
	case 'E':
	//file must exist
	if ((strcasecmp(Type, "exists")==0) && (access(ptr, F_OK) !=0)) result=FALSE;
	break;

	case 'l':
	case 'L':
	//lockfile
	if ((strcasecmp(Type, "lock")==0) && (! CreateLockFile(ptr, 1)) ) result=FALSE;
	break;
}

Destroy(Type);
return(result);
}




int RunInvokeCommand(STREAM *S, const char *Command, ListNode *Vars, ListNode *Config, TPeerInfo *Peer)
{
ListNode *Curr;
char *Tempstr=NULL;

Curr=ListGetNext(Config);
while (Curr)
{
	if (Curr->ItemType==ITEM_VALUE)
	{
	if (strcmp(Curr->Tag, "require")==0) 
	{
		if (! CheckRequirement(Curr->Item, S, Peer, Vars)) 
		{
			Destroy(Tempstr);
			return(FALSE);
		}
	}
	else if (strcmp(Curr->Tag, "chuser")==0) SwitchUser((const char *) Curr->Item);
	else if (strcmp(Curr->Tag, "chgrp")==0) SwitchGroup((const char *) Curr->Item);
	else if (strcmp(Curr->Tag, "bookout")==0) BookoutFileToClient(S, (const char *) Curr->Item, Vars);
	else if (strcmp(Curr->Tag, "bookin")==0) BookinFileFromClient(S, (const char *) Curr->Item, Vars);
	else if (strcmp(Curr->Tag, "clientrun")==0) 
	{
		Tempstr=MCopyStr(Tempstr, "clientrun ",(const char *) Curr->Item,"\n",NULL);
		STREAMWriteLine(Tempstr, S);
//		BookoutFileToClient(S, (const char *) Curr->Item, Vars);
	}
	else if (strcmp(Curr->Tag, "run")==0) RunCommand(S, (const char *) Curr->Item, Vars, Config);
	}
	waitpid(-1, NULL, WNOHANG);
	Curr=ListGetNext(Curr);
}


Destroy(Tempstr);
return(FALSE);
}


int RunInvoke(STREAM *S, const char *Command, ListNode *Vars, TPeerInfo *Peer)
{
ListNode *Curr;

Curr=ListGetNext(Config);
while (Curr)
{
	if (strcmp(Curr->Tag, Command)==0) return(RunInvokeCommand(S, Command, Vars, (ListNode *) Curr->Item, Peer));
	Curr=ListGetNext(Curr);
}
return(FALSE);
}




static int HandleInvoke(STREAM *S, TPeerInfo *Peer, const char *Details)
{
char *Command=NULL, *Name=NULL, *Value=NULL;
ListNode *Vars;
const char *ptr;

Vars=ListCreate();
ptr=Details;
while (isspace(*ptr)) ptr++;
ptr=GetToken(ptr, "\\S", &Command, GETTOKEN_QUOTES);
while (ptr)
{
	ptr=GetNameValuePair(ptr, " ", "=", &Name, &Value);
	while (ptr)
	{
	StripTrailingWhitespace(Value);
	StripQuotes(Value);
	SetVar(Vars, Name, Value);
	ptr=GetNameValuePair(ptr, " ", "=", &Name, &Value);
	}
}


SetVar(Vars, "invoke:user", Peer->UserName);
SetVar(Vars, "invoke:group", Peer->GroupName);
Value=FormatStr(Value,"%d", Peer->uid);
SetVar(Vars, "invoke:uid", Value);
Value=FormatStr(Value,"%d", Peer->gid);
SetVar(Vars, "invoke:gid", Value);

if (FindPermission(S, Peer, Command, "")) RunInvoke(S, Command, Vars, Peer);
else STREAMWriteLine("FAIL: Not authorized\n", S);

ListDestroy(Vars, Destroy);

Destroy(Command);
Destroy(Name);
Destroy(Value);
}


static int HandleExec(STREAM *PeerS, TPeerInfo *Peer, const char *Details)
{
char *Command=NULL;
const char *ptr;

ptr=GetToken(Details, "\\S",&Command,0);
if (FindPermission(PeerS, Peer, Command, ptr))
{
	RunCommand(PeerS, Details, NULL, NULL);
}
else STREAMWriteLine("FAIL: Not authorized\n", PeerS);

Destroy(Command);
}


static int HandleClientRequest(STREAM *S, TPeerInfo *Peer, const char *AuthTypes)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr;
int RetVal=FALSE;

Tempstr=STREAMReadLine(Tempstr, S);
StripTrailingWhitespace(Tempstr);
if (StrValid(Tempstr))
{
	ptr=GetToken(Tempstr, "\\S|:", &Token, GETTOKEN_MULTI_SEP);
	if (strcasecmp(Token, "AUTH")==0) HandleClientAuth(S, Peer, ptr, AuthTypes);
	else if (strcasecmp(Token, "EXEC")==0) HandleExec(S, Peer, ptr);
	else if (strcasecmp(Token, "INVOKE")==0) HandleInvoke(S, Peer, ptr);
	RetVal=TRUE;
}

Destroy(Tempstr);
Destroy(Token);

return(RetVal);
}




void HandleClient(STREAM *S)
{
TPeerInfo *PI;
pid_t pid;

pid=fork();
if (pid==0)
{
PI=GetPeerInfo(S->in_fd);

//get request from client
HandleClientRequest(S, PI, "");

DestroyPeerInfo(PI);
STREAMClose(S);
_exit(0);
}

STREAMClose(S);
waitpid(-1, NULL, WNOHANG);
}


int ParseCommandLine(int argc, char *argv[])
{
CMDLINE *CL;
const char *ptr=NULL;
int AppFlags=0;

CL=CommandLineParserCreate(argc, argv);
ptr=CommandLineNext(CL);
while (ptr)
{
if (strcmp(ptr,"-nodemon")==0) Flags |= FLAG_NODEMON; 
else if (strcmp(ptr,"-k")==0) AppFlags |= APPFLAG_KEY; 
else if (strcmp(ptr,"-version")==0) 
{
	printf("invocation version: %s\n", VERSION);
	exit(1);
}
else if (strcmp(ptr,"--version")==0) 
{
	printf("invocation version: %s\n", VERSION);
	exit(1);
}
ptr=CommandLineNext(CL);
}

}


int main(int argc, char *argv[])
{
ListNode *Listeners;
STREAM *S, *Client;

ParseCommandLine(argc, argv);
signal(SIGPIPE, SIG_IGN);

LoadConfig("/etc/invoke.conf");

Listeners=ListCreate();
S=STREAMServerInit("unix:/dev/invocation");
chmod("/dev/invocation", 0666);
if (S) ListAddItem(Listeners, S);

if (! (Flags & FLAG_NODEMON)) demonize();

while (1)
{
S=STREAMSelect(Listeners, NULL);
if (S) 
{
	Client=STREAMServerAccept(S);
	HandleClient(Client);
}
}

return(0);
}
