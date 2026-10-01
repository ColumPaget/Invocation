#ifndef INVOKED_AUTH_H
#define INVOKED_AUTH_H

#include "common.h"

#define USER_UNKNOWN -1

int AuthCheck(const char *AuthTypes, const char *UserName, const char *Password);


#endif
