
#ifndef INVOCATION_PEER_H
#define INVOCATION_PEER_H

#include "common.h"

#define PEER_AUTHENTICATED 1

typedef struct
{
int Flags;
int pid;
int uid;
int gid;
char *UserName;
char *GroupName;
char *ExePath;
char *ExeMD5;
char *HostName;
} TPeerInfo;


TPeerInfo *GetPeerInfo(int sock);
void DestroyPeerInfo(void *P);

#endif
