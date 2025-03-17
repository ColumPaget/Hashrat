#include "Users.h"

int LookupUID(const char *User)
{
    struct passwd *pwent;

    if (! StrValid(User)) return(-1);
    pwent=getpwnam(User);
    if (! pwent) return(-1);
    return(pwent->pw_uid);
}


int LookupGID(const char *Group)
{
    struct group *grent;

    if (! StrValid(Group)) return(-1);
    grent=getgrnam(Group);
    if (! grent) return(-1);
    return(grent->gr_gid);
}


const char *LookupUserName(uid_t uid)
{
    struct passwd *pwent;

    pwent=getpwuid(uid);
    if (! pwent) return("");
    return(pwent->pw_name);
}


const char *LookupGroupName(gid_t gid)
{
    struct group *grent;

    grent=getgrgid(gid);
    if (! grent) return("");
    return(grent->gr_name);
}


int SwitchUID(int uid)
{
    struct passwd *pw;

    pw=getpwuid(uid);

    //must initgroups before switch user, as the user we switch too
    //may not have permission to do so
#ifdef HAVE_INITGROUPS
    if (pw) initgroups(pw->pw_name, 0);
#endif

#ifdef HAVE_SETRESUID
    if ((uid==-1) || (setresuid(uid,uid,uid) !=0))
#else
    if ((uid==-1) || (setreuid(uid,uid) !=0))
#endif
    {
        RaiseError(ERRFLAG_ERRNO, "SwitchUID", "Switch user failed. uid=%d",uid);
        if (LibUsefulGetBool("SwitchUserAllowFail")) return(FALSE);
        exit(1);
    }

    if (pw)
    {
        setenv("HOME",pw->pw_dir,TRUE);
        setenv("USER",pw->pw_name,TRUE);

    }
    return(TRUE);
}


int SwitchUser(const char *NewUser)
{
    int uid;

    uid=LookupUID(NewUser);
    if (uid==-1) return(FALSE);
    return(SwitchUID(uid));
}


int SwitchGID(int gid)
{
    if ((gid==-1) || (setgid(gid) !=0))
    {
        RaiseError(ERRFLAG_ERRNO, "SwitchGID", "Switch group failed. gid=%d",gid);
        if (LibUsefulGetBool("SwitchGroupAllowFail")) return(FALSE);
        exit(1);
    }
    return(TRUE);
}

int SwitchGroup(const char *NewGroup)
{
    int gid;

    gid=LookupGID(NewGroup);
    if (gid==-1) return(FALSE);
    return(SwitchGID(gid));
}




const char *GetUserHomeDir(const char *User)
{
    struct passwd *pwent;

    if (! StrValid(User)) return(NULL);
    pwent=getpwnam(User);
    if (! pwent)
    {
        RaiseError(ERRFLAG_ERRNO, "getpwuid","Failed to get info for user [%s]", User);
        return(NULL);
    }
    return(pwent->pw_dir);
}



const char *GetCurrUserHomeDir()
{
    struct passwd *pwent;

    pwent=getpwuid(getuid());
    if (! pwent)
    {
        RaiseError(ERRFLAG_ERRNO, "getpwuid","Failed to get info for current user");
        return(NULL);
    }
    return(pwent->pw_dir);
}

