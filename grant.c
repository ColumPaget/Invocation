#include "common.h"
#include "appdb.h"

int ParseCommandLine(CMDLINE *CL, char **Grants, char **Program, char **Users, char **Groups)
{
const char *ptr;

*Grants=CopyStr(*Grants, "");
*Program=CopyStr(*Program, "");
*Users=CopyStr(*Users, "");
*Groups=CopyStr(*Groups, "");
ptr=CommandLineNext(CL);
while (ptr)
{
if (strcmp(ptr, "-u")==0) *Users=MCatStr(*Users, CommandLineNext(CL), ",", NULL);
else if (strcmp(ptr, "-g")==0) *Groups=MCatStr(*Groups, CommandLineNext(CL), ",", NULL);
else if (! StrValid(*Program)) *Program=CopyStr(*Program, ptr);
else *Grants=MCatStr(*Grants, ptr, ",", NULL);

ptr=CommandLineNext(CL);
}
}

int main(int argc, char *argv[])
{
CMDLINE *CL;
char *Grants=NULL, *Program=NULL, *Users=NULL, *Groups=NULL;
const char *ptr=NULL;

CL=CommandLineParserCreate(argc, argv);

ParseCommandLine(CL, &Grants, &Program, &Users, &Groups);
AppDBAdd(Grants, Users, Groups, Program);

Destroy(Grants);
Destroy(Program);
Destroy(Users);
Destroy(Groups);

return(0);
}
