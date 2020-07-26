#ifndef INVOCATION_BOOK_INOUT_H
#define INVOCATION_BOOK_INOUT_H

/* These functions relate to the invocation book in/out system that can send a file to a client app and/or
read one back into place from the client app. This means clients can be given access to files normally owned
by root (e.g. crontabs) without the user and their file editor program ever having root permissions
*/
  
#include "common.h"

void BookoutFileToClient(STREAM *Client, const char *Path, ListNode *Vars);
void BookinFileFromClient(STREAM *Client, const char *iPath, ListNode *Vars);

#endif
