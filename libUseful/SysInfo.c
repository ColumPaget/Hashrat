#include "SysInfo.h"
#include "Socket.h" // for MAX_HOST_NAME

#include <sys/utsname.h>

#ifdef linux
#include <sys/sysinfo.h>
#endif

#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>


char *OSSysInfoInterfaces(char *RetStr)
{
struct if_nameindex *interfaces;
int i;

RetStr=CopyStr(RetStr, "");
interfaces=if_nameindex();
if (interfaces)
{
for (i=0; interfaces[i].if_name != NULL; i++)
{
	RetStr=MCatStr(RetStr, interfaces[i].if_name, " ", NULL);
}
if_freenameindex(interfaces);
}

return(RetStr);
}



//This is a convinience function for use by modern languages like
//lua that have an 'os' object that returns information
const char *OSSysInfoString(int Info)
{
    static struct utsname UtsInfo;
    struct passwd *pw;
    const char *ptr;
		static char *buf=NULL;

    uname(&UtsInfo);

    switch (Info)
    {
    case OSINFO_TYPE:
        return(UtsInfo.sysname);
        break;

    case OSINFO_ARCH:
        return(UtsInfo.machine);
        break;

    case OSINFO_RELEASE:
        return(UtsInfo.release);
        break;

    case OSINFO_HOSTNAME:
				buf=SetStrLen(buf, HOST_NAME_MAX);
				gethostname(buf, HOST_NAME_MAX);
        return(buf);
        break;

    case OSINFO_DOMAINNAME:
				buf=SetStrLen(buf, HOST_NAME_MAX);
				getdomainname(buf, HOST_NAME_MAX);
        return(buf);
        break;

    case OSINFO_HOMEDIR:
        pw=getpwuid(getuid());
        if (pw) return(pw->pw_dir);
        break;

    case OSINFO_TMPDIR:
        ptr=getenv("TMPDIR");
        if (! ptr) ptr=getenv("TEMP");
        if (! ptr) ptr="/tmp";
        if (ptr) return(ptr);
        break;

		case OSINFO_INTERFACES:
			//don't just return output of function, as buf is static we must update it
			buf=OSSysInfoInterfaces(buf);
			return(buf);
		break;


        /*
        case OSINFO_USERINFO:
          pw=getpwuid(getuid());
          if (pw)
          {
            MuJSNewObject(TYPE_OBJECT);
            MuJSNumberProperty("uid",pw->pw_uid);
            MuJSNumberProperty("gid",pw->pw_gid);
            MuJSStringProperty("username",pw->pw_name);
            MuJSStringProperty("shell",pw->pw_shell);
            MuJSStringProperty("homedir",pw->pw_dir);
          }
        break;
        }
        */

    }


    return("");
}


//This is a convienice function for use by modern languages like
//lua that have an 'os' object that returns information
size_t OSSysInfoLong(int Info)
{
#ifdef linux
    struct sysinfo SysInfo;

    sysinfo(&SysInfo);
    switch (Info)
    {
    case OSINFO_UPTIME:
        return((size_t) SysInfo.uptime);
        break;

    case OSINFO_TOTALMEM:
        return((size_t) (SysInfo.totalram * SysInfo.mem_unit));
        break;

    case OSINFO_FREEMEM:
        return((size_t) (SysInfo.freeram * SysInfo.mem_unit));
        break;

    case OSINFO_BUFFERMEM:
        return((size_t) (SysInfo.bufferram * SysInfo.mem_unit));
       break;

		case OSINFO_TOTALSWAP:
			 	return((size_t) (SysInfo.totalswap * SysInfo.mem_unit));
			break;

		case OSINFO_FREESWAP:
			 	return((size_t) (SysInfo.freeswap * SysInfo.mem_unit));
				break;

    case OSINFO_PROCS:
        return((size_t) SysInfo.procs);
        break;

		case OSINFO_LOAD1MIN:
        return((size_t) SysInfo.loads[0]);
				break;

		case OSINFO_LOAD5MIN:
        return((size_t) SysInfo.loads[1]);
				break;

		case OSINFO_LOAD15MIN:
        return((size_t) SysInfo.loads[2]);
				break;
    }

#endif
    return(0);
}
