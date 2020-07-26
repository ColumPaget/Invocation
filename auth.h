#ifndef INVOKED_AUTH_H
#define INVOKED_AUTH_H

#include "common.h"

#define USER_UNKNOWN -1

//Do authentication. 'PAMTypes' is a comma list of service names to authenticate
//against, like 'login', 'httpd' or 'other'
int AuthPAM(const char *PAMTypes, const char *UserName, const char *Pass);

//does account exist?
int AuthPAMCheckAccount(const char *UserName);

//call this to cleanup after either of the above has been used 
void AuthPAMClose();

#endif
