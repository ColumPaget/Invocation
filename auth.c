#include "auth.h"

/* PAM works in a bit of a strange way, insisting on having a callback */
/* function that it uses to prompt for the password. We have arranged  */
/* to have the password passed in as the 'appdata' arguement, so this  */
/* function just passes it back!                                       */

#ifdef HAVE_LIBPAM

#include <security/pam_appl.h>
static pam_handle_t *pamh=NULL;


int PAMConvFunc(int NoOfMessages, const struct pam_message **messages,
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


int PAMStart(const char *PAMTypes, const char *UserName, const char *Password)
{
static struct pam_conv  PAMConvStruct = {PAMConvFunc, NULL };
int result=PAM_PERM_DENIED;
const char *ptr;
char *Token=NULL;

  PAMConvStruct.appdata_ptr=(void *) Password;

	ptr=GetToken(PAMTypes, ",", &Token, GETTOKEN_QUOTES);
	while (ptr)
  {
    result=pam_start(Token, UserName, &PAMConvStruct, &pamh);
		if (result==PAM_SUCCESS) break;
		ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
  }

	Destroy(Token);

  if (result==PAM_SUCCESS)
  {
  pam_set_item(pamh,PAM_RUSER, UserName);
  pam_set_item(pamh,PAM_RHOST,"");
  return(TRUE);
  }

  return(FALSE);
}
#endif



int AuthPAM(const char *PAMTypes, const char *UserName, const char *Pass)
{
#ifdef HAVE_LIBPAM
static struct pam_conv  PAMConvStruct = {PAMConvFunc, NULL };
int result;

result=PAMStart(PAMTypes, UserName, Pass);
if (result != PAM_SUCCESS)
{
  pam_end(pamh,result);
  return(USER_UNKNOWN);
}

result=pam_authenticate(pamh, 0);

if (result==PAM_SUCCESS)
{
//  if (Settings.Flags & FLAG_LOG_VERBOSE) LogToFile(Settings.LogPath,"AUTH: UserName '%s' Authenticated via PAM.",UserName);
 return(TRUE);
}
#endif

return(FALSE);
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



void AuthPAMClose()
{
#ifdef HAVE_LIBPAM
  if (pamh)
  {
  pam_close_session(pamh, 0);
  pam_end(pamh,PAM_SUCCESS);
  }
#endif
}
