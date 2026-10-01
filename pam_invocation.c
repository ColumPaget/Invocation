
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define FALSE 0
#define TRUE 1

//Define which PAM interfaces we provide. In this case we are
//only going to provide an authentication interface, i.e. one
//that decides if a login in allowed or not
#define PAM_SM_AUTH

// We do not supply these
/*
#define PAM_SM_ACCOUNT
#define PAM_SM_PASSWORD
#define PAM_SM_SESSION
*/

// Include PAM headers
#include <security/pam_appl.h>
#include <security/pam_modules.h>




typedef struct
{
    char *auth_sock_path;
    char *prompt;
} TSettings;


void Destroy(void *item)
{
    if (item) free(item);
}

int StrLen(const char *Str)
{
    if (! Str) return(0);
    return(strlen(Str));
}

char *CatStr(char *Dest, const char *Src)
{
    int len;

    len=StrLen(Dest) + StrLen(Src)+1;

    Dest=(char *) realloc(Dest, len);
    strcat(Dest, Src);

    return(Dest);
}


char *CopyStr(char *Dest, const char *Src)
{
    Dest=(char *) realloc(Dest, StrLen(Src) + 1);
    strcpy(Dest, Src);
    return(Dest);
}

void StripTrailingWhitespace(char *str)
{
    size_t len;
    char *ptr;

    len=StrLen(str);
    if (len==0) return;
    for(ptr=str+len-1; (ptr >= str) && isspace(*ptr); ptr--) *ptr='\0';
}


void StripLeadingWhitespace(char *str)
{
    char *ptr, *start=NULL;

    if (! str) return;
    for(ptr=str; *ptr !='\0'; ptr++)
    {
        if ((! start) && (! isspace(*ptr))) start=ptr;
    }

    if (!start) start=ptr;
    memmove(str,start,ptr+1-start);
}


//this accepts a void * purely because I'm used to the libUseful way
//of doing things where sometimes an item is in a list, and is a void *
//because the list can hold untyped items
void TSettingsDestroy(void *p_Settings)
{
    TSettings *Settings;

    if (p_Settings)
    {
        Settings=(TSettings *) p_Settings;
        Destroy(Settings->prompt);
        Destroy(Settings->auth_sock_path);
        free(Settings);
    }

}



//this function sets up the 'sa' object for a unix socket
//we copy our path into the sa->sun_path field of the 'sa' object
//however, this is not a standard size in all implementations
//so we have to do a strange dance
void sun_set_path(struct sockaddr_un *sa, const char *Path)
{
    int len, val;
    char *ptr1, *ptr2;

    memset(sa,0,sizeof(struct sockaddr_un));
    sa->sun_family=AF_UNIX;
    ptr1=sa->sun_path;
    ptr2=(char *) sa; //sa is already a pointer, so we don't need &

    //find the length of sa->sun_path. We do this by finding
    //the offset of sun_path (ptr1-ptr2) and then deleting that
    //from the size of the sockaddr_un ('sa') object. Assuming that
    //sockaddr_un is the last field in the sockaddr_un, then
    //what remains is the available space to copy into
    len=sizeof(struct sockaddr_un) - (ptr1-ptr2);

    if (len > 0)
    {
        len--; //force room for terminating \0

        //all hail strlcpy, makes life easier
#ifdef strlcpy
        strlcpy(sa->sun_path,Path,len);

        //else grrrr
#else
        val=StrLen(Path);
        if (val < len) len=val;
        strncpy(sa->sun_path,Path,len);
        sa->sun_path[len]='\0';
#endif

    }

}


int OpenUnixSocket(const char *Path)
{
    int sock;
    struct sockaddr_un sa;
    char *Tempstr=NULL;
    int val;

    if (access(Path, F_OK) != 0) return(-1);

    sock=socket(PF_UNIX, SOCK_STREAM, FALSE);

    if (sock==-1) return(-1);

    sun_set_path(&sa, Path);
    val=sizeof(sa);
    if (connect(sock, (struct sockaddr *) &sa, val)==0)
    {
        Tempstr=CopyStr(Tempstr, "STATUS\n");
        write(sock, Tempstr, StrLen(Tempstr));

        Tempstr=(char *) realloc(Tempstr, 255);
        val=read(sock, Tempstr, 254);
        if (strncmp(Tempstr, "READY ", 6) != 0)
        {
            close(sock);
            sock=-1;
        }
    }
    else
    {
        close(sock);
        sock=-1;
    }

    Destroy(Tempstr);

    return(sock);
}



char *PAMGetAuthtok(char *RetStr, pam_handle_t *pamh, const char *Prompt)
{
    struct pam_conv *conv;
    struct pam_message msg;
    const struct pam_message *msgp;
    struct pam_response *resp=NULL;
    const char *pam_authtok=NULL;
    int len;

    RetStr=CopyStr(RetStr,"");

//This gets a 'cached' authtoken (password). On some systems (OpenPAM) it should handle
//the pam conversation too. The password is a constant string in the PAM environment, so
//it does not need to be freed. We return a copy of it instead.
    if (pam_get_item(pamh, PAM_AUTHTOK, (const void **) &pam_authtok) == PAM_SUCCESS)
    {
        if (StrLen(pam_authtok)) RetStr=CopyStr(RetStr, pam_authtok);
    }

//if the above pam_get_item returned nothing, then we'll have to try doing the pam
//conversation 'by hand'.
#ifndef OPENPAM
    if (! StrLen(RetStr))
    {

        if (pam_get_item(pamh, PAM_CONV, (const void **)&conv) != PAM_SUCCESS) return(RetStr);

        msg.msg_style = PAM_PROMPT_ECHO_OFF;
        msg.msg = Prompt;
        msgp = &msg;

        if (conv)
        {
            if ((*conv->conv)(1, &msgp, &resp, conv->appdata_ptr) == PAM_SUCCESS)
            {
                //if the string starts with a printable character then we got a password
                if (resp && resp->resp && isprint(*(resp->resp))) RetStr=CopyStr(RetStr, resp->resp);
            }
        }

        if (resp)
        {
            if (resp->resp)
            {
//  ScrubString(resp->resp);
                Destroy(resp->resp);
            }
            Destroy(resp);
        }
    }
#endif

    StripTrailingWhitespace(RetStr);
    return(RetStr);
}


TSettings *ParseSettings(int argc, const char **argv)
{
    TSettings *Settings=NULL;
    int i;

    Settings=(TSettings *) calloc(1, sizeof(TSettings));
    Settings->prompt=CopyStr(Settings->prompt, "password: ");
    Settings->auth_sock_path=CopyStr(Settings->auth_sock_path, "/dev/invocation");
    for (i=1; i < argc; i++)
    {
        if (strncasecmp(argv[i], "prompt=", 7) ==0) Settings->prompt=CopyStr(Settings->prompt, argv[++i]);
        if (strncasecmp(argv[i], "sockpath=", 10) ==0) Settings->auth_sock_path=CopyStr(Settings->auth_sock_path, argv[++i]);
    }

    return(Settings);
}


int AuthTransact(TSettings *Settings, int auth_sock, const char *pam_authtok)
{
    char *Tempstr=NULL;
    int len;
    int RetVal=FALSE;

    if (auth_sock > -1)
    {
        Tempstr=CopyStr(Tempstr, "AUTH password='");
        Tempstr=CatStr(Tempstr, pam_authtok);
        Tempstr=CatStr(Tempstr, "'\n");
        write(auth_sock, Tempstr, StrLen(Tempstr));
        Tempstr=(char *) realloc(Tempstr, 255);
        len=read(auth_sock, Tempstr, 254);

        Tempstr[len]='\0';

        StripTrailingWhitespace(Tempstr);
        if (strncmp(Tempstr, "OKAY:", 5)==0) RetVal=TRUE;
    }
    Destroy(Tempstr);

    return(RetVal);
}


// PAM entry point for authentication. This function gets called by pam when
//a login occurs. argc and argv work just like argc and argv for the 'main'
//function of programs, except they pass in the options defined for this
//module in the pam configuration files in /etc/pam.conf or /etc/pam.d/
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    int PamResult=PAM_IGNORE;
    char *Tempstr=NULL;
    TSettings *Settings;

    //These are defined as 'const char' because they passwd to us from the parent
    //library. When we called pam_get_<whatever> the pam library passes pointers
    //to strings in it's own code. Thus we must not change or free them
    const char *pam_user = NULL, *pam_service=NULL;
    char *pam_authtok=NULL;
    int sock=-1;

    openlog("pam_invocation", 0, LOG_AUTH);
    Settings=ParseSettings(argc, argv);
    sock=OpenUnixSocket(Settings->auth_sock_path);

    if (sock == -1)
    {
        syslog(LOG_ERR, "pam_invocation: failed to open invocation socket: %s", Settings->auth_sock_path);
        closelog();

        return(PAM_IGNORE);
    }


    if (pam_get_item(pamh, PAM_SERVICE, (const void **) &pam_service) != PAM_SUCCESS)
    {
        syslog(LOG_ERR, "Failed to get pam_service");
        closelog();
        return(PAM_IGNORE);
    }

    syslog(LOG_ERR, "pam_invocation: startup %s", pam_service);


    //get the user. If something goes wrong we return PAM_IGNORE. This tells
    //pam that our module failed in some way, so ignore it. Perhaps we should
    //return PAM_PERM_DENIED to deny login, but this runs the risk of a broken
    //module preventing anyone from logging into the system!
    if ((pam_get_user(pamh, &pam_user, NULL) != PAM_SUCCESS) || (pam_user==NULL))
    {
        syslog(LOG_ERR, "pam_invocation: Failed to get pam_user");
        closelog();
        return(PAM_IGNORE);
    }

    pam_authtok=PAMGetAuthtok(pam_authtok, pamh, Settings->prompt);
    if (! StrLen(pam_authtok))
    {
        //blank authentication token. This could mean that the user is logging in
        //using public/private keys. As we have no way of detecting that, and we
        //have no auth token to process, we return PAM_IGNORE
        syslog(LOG_ERR, "pam_invocation: blank authtok", pam_authtok);
        closelog();
        return(PAM_IGNORE);
    }

    if (AuthTransact(Settings, sock, pam_authtok)) PamResult=PAM_SUCCESS;

    TSettingsDestroy(Settings);
    Destroy(Tempstr);

    return(PamResult);
}


//We do not provide any of the below functions, we could just leave them out
//but apparently it's considered good practice to supply them and return
//'PAM_IGNORE'

//PAM entry point for starting sessions. This is called after a user has
//passed all authentication. It allows a PAM module to perform certain tasks
//on login, like recording the login occured, or printing a message of the day
int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return(PAM_IGNORE);
}


//PAM entry point for ending sessions. This is called when a user logs out
//It allows a PAM module to perform certain tasks on logout
//like recording the logout occured, or clearing up temporary files
int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return(PAM_IGNORE);
}

// PAM entry point for 'account management'. This decides whether a user
// who has already been authenticated by pam_sm_authenticate should be
// allowed to log in (it considers other things than the users password)
// Really this is what we should have used here
int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return(PAM_IGNORE);
}


//PAM entry point for setting 'credentials' or properties of the user
//If our module stores or produces extra information about a user (e.g.
//a kerberous ticket or geolocation value) then it will pass this information
//to a PAM aware program in this call
int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return(PAM_IGNORE);
}

// PAM entry point for changing passwords. If our module stores passwords
// then this will be called whenever one needs changing
int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return(PAM_IGNORE);
}


//I couldn't find any documentation on this. I think it notifies PAM of our
//module name
#ifdef PAM_MODULE_ENTRY
PAM_MODULE_ENTRY("pam_invocation");
#endif

