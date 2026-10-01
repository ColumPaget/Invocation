#include "auth.h"

/* PAM works in a bit of a strange way, insisting on having a callback */
/* function that it uses to prompt for the password. We have arranged  */
/* to have the password passed in as the 'appdata' arguement, so this  */
/* function just passes it back!                                       */

#ifdef HAVE_LIBPAM

#include <security/pam_appl.h>
static pam_handle_t *pamh=NULL;


static int PAMConvFunc(int NoOfMessages, const struct pam_message **messages,
                       struct pam_response **responses, void *appdata)
{
    int count;
    const struct pam_message *mess;
    struct pam_response *resp;

    *responses=(struct pam_response *) calloc(NoOfMessages,sizeof(struct pam_response));

    mess=*messages;
    resp=*responses;

    for (count=0; count < NoOfMessages; count++)
    {
        if ((mess->msg_style==PAM_PROMPT_ECHO_OFF) ||
                (mess->msg_style==PAM_PROMPT_ECHO_ON))
        {
            resp->resp=CopyStr(NULL,(char *) appdata);
            resp->resp_retcode=0;
        }
        mess++;
        resp++;
    }

    return(PAM_SUCCESS);
}


static int PAMStart(const char *PAMType, const char *UserName, const char *Password)
{
    static struct pam_conv  PAMConvStruct = {PAMConvFunc, NULL };
    int result=PAM_PERM_DENIED;

    PAMConvStruct.appdata_ptr=(void *) Password;
    if (pam_start(PAMType, UserName, &PAMConvStruct, &pamh) == PAM_SUCCESS)
    {
        pam_set_item(pamh,PAM_RUSER, UserName);
        pam_set_item(pamh,PAM_RHOST,"");
        return(TRUE);
    }

    return(FALSE);
}

static void PAMClose(int PAMResult)
{
    if (pamh)
    {
        pam_close_session(pamh, 0);
        pam_end(pamh, PAMResult);
        pamh=NULL;
    }
}


#endif



static int AuthPAM(const char *PAMType, const char *UserName, const char *Pass)
{
    int RetVal=FALSE;

#ifdef HAVE_LIBPAM
    static struct pam_conv  PAMConvStruct = {PAMConvFunc, NULL };
    int result;

    result=PAMStart(PAMType, UserName, Pass);
    if (result == PAM_SUCCESS)
    {
        result=pam_authenticate(pamh, 0);
        if (result==PAM_SUCCESS) RetVal=TRUE;
    }
    else RetVal=USER_UNKNOWN;

    PAMClose(result);
#endif

    return(RetVal);
}




int AuthPAMCheckAccount(const char *UserName)
{
#ifdef HAVE_LIBPAM

    if (! pamh)
    {
        if (! PAMStart("login", UserName, "")) return(FALSE);
    }

    if (pam_acct_mgmt(pamh, 0)==PAM_SUCCESS)
    {
        pam_open_session(pamh, 0);
        return(TRUE);
    }
#endif

    return(FALSE);
}





//Authentications that don't use PAM
static int AuthPasswdFile(const char *User, const char *Password)
{
    struct passwd *pass_struct;
    const char *ptr;

    pass_struct=getpwnam(User);
    if (pass_struct==NULL) return(USER_UNKNOWN);

#ifdef HAVE_LIBCRYPT

    if (pass_struct->pw_passwd && Password)
    {
        ptr=crypt(Password, pass_struct->pw_passwd);
        if (ptr && (strcmp(pass_struct->pw_passwd, ptr)==0))
        {
            return(TRUE);
        }
    }

#endif

    return(FALSE);
}


static int AuthShadowFile(const char *User, const char *Password)
{
    char *sptr, *eptr, *Salt=NULL, *Digest=NULL;
    int result=FALSE;

#ifdef HAVE_SHADOW_H
#include <shadow.h>
    struct spwd *pass_struct=NULL;

    pass_struct=getspnam(User);

    if (pass_struct==NULL) return(USER_UNKNOWN);

#ifdef HAVE_LIBCRYPT
    Digest=CopyStr(Digest, crypt(Password, pass_struct->sp_pwdp));
    if (strcmp(Digest, pass_struct->sp_pwdp)==0) result=TRUE;
#endif


#endif
    Destroy(Salt);
    Destroy(Digest);

    return(result);
}


int AuthCheck(const char *AuthTypes, const char *UserName, const char *Password)
{
    char *Type=NULL;
    const char *ptr;
    int result=FALSE;

    ptr=GetToken(AuthTypes, ",", &Type, GETTOKEN_QUOTES);
    while (ptr)
    {
        if (strcasecmp(Type, "passwd")==0) result=AuthPasswdFile(UserName, Password);
        else if (strcasecmp(Type, "shadow")==0) result=AuthShadowFile(UserName, Password);
        else result=AuthPAM(Type, UserName, Password);

        if (result == TRUE) break;

        ptr=GetToken(ptr, ",", &Type, GETTOKEN_QUOTES);
    }

    Destroy(Type);

//do this check because Auth Functions can return other values than TRUE
    if (result==TRUE) return(TRUE);
    return(FALSE);
}
