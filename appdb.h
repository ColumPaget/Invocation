#ifndef INVOCATION_APPDB_H
#define INVOCATION_APPDB_H

#include "common.h"

#define APPFLAG_KEY 1

typedef struct
{
char *Type;
char *User;
char *Group;
char *Program;
char *Hash;
size_t Size;
} TGrant;

int AppDBCheck(const char *Grant, const char *AppPath);
void AppDBAdd(const char *Grant, const char *User, const char *Group, const char *AppPath);

#endif
