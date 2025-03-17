#include "../libUseful.h"

main()
{
ProcessApplyConfig("nosu");
printf("You should now be running in a shell that cannot su/sudo\n");
printf("Type 'exit' to leave it\n");
setenv("PS1", "restricted: ", TRUE);
SwitchProgram("/bin/bash", "");
}
