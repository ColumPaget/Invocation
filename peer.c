#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/param.h>
#include "peer.h"


void DestroyPeerInfo(void *p_Info)
{
    TPeerInfo *PeerInfo;

    PeerInfo=(TPeerInfo *) p_Info;
    Destroy(PeerInfo->UserName);
    Destroy(PeerInfo->GroupName);
    Destroy(PeerInfo->ExePath);
    Destroy(PeerInfo->ExeMD5);
    Destroy(PeerInfo->HostName);
    Destroy(PeerInfo);
}



char *GetClientMD5Sum(char *RetStr, int pid)
{
    char *Tempstr=NULL;

    Tempstr=FormatStr(Tempstr, "/proc/%d/exe",pid);
    HashFile(&RetStr, "md5", Tempstr, ENCODE_HEX);
    Destroy(Tempstr);

    return(RetStr);
}



char *GetExePath(char *ReturnBuff, int pid)
{
    char *Tempstr=NULL;
    STREAM *S;

    Tempstr=FormatStr(Tempstr, "/proc/%d/exe",pid);
    ReturnBuff=SetStrLen(ReturnBuff,MAXPATHLEN);
    realpath(Tempstr,ReturnBuff);
    Destroy(Tempstr);
    return(ReturnBuff);
}


TPeerInfo *GetPeerInfo(int sock)
{
    struct ucred SockCreds;
    TPeerInfo *PeerInfo;
    int salen;
    struct passwd *User;

    PeerInfo=(TPeerInfo *) calloc(1, sizeof(TPeerInfo));
    salen=sizeof(struct ucred);
    getsockopt(sock, SOL_SOCKET, SO_PEERCRED, & SockCreds, &salen);

    PeerInfo->pid=SockCreds.pid;
    PeerInfo->uid=SockCreds.uid;
    PeerInfo->gid=SockCreds.gid;

    PeerInfo->UserName=CopyStr(PeerInfo->UserName,LookupUserName(PeerInfo->uid));
    PeerInfo->GroupName=CopyStr(PeerInfo->GroupName,LookupGroupName(PeerInfo->gid));
    PeerInfo->HostName=CopyStr(PeerInfo->HostName,"???");

    PeerInfo->ExePath=GetExePath(PeerInfo->ExePath, PeerInfo->pid);
    PeerInfo->ExeMD5=GetClientMD5Sum(PeerInfo->ExeMD5, PeerInfo->pid);
//PeerInfo->TTY=GetProgTTY(PeerInfo->TTY,PeerInfo->pid);

    return(PeerInfo);
}

