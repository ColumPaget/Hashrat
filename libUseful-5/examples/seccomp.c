#include "../libUseful.h"
#include <sys/ptrace.h>

main()
{
pid_t pid;
STREAM *S;


ProcessApplyConfig("security='client'");
pid=fork();
if (pid == 0)
{

S=STREAMOpen("unix:/tmp/.X11-unix/X0", "");
STREAMClose(S);

S=STREAMOpen("http://www.google.com/", "");
printf("After GOOGLE: %s\n", STREAMGetValue(S, "HTTP:ResponseCode"));
STREAMClose(S);

S=STREAMServerNew("tcp:0.0.0.0:2048", "");
printf("Server: %d\n", S);
STREAMClose(S);

chroot(".");
printf("After chroot\n");

ptrace(PTRACE_CONT, getppid(), 0, 0);
printf("After ptrace\n");

exit(1);
}
waitpid(pid, 0, 0);

}
