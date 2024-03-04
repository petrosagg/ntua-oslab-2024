#include <unistd.h>
#include <stdio.h>
 
void zing(){
 char *login_name = getlogin();
 printf("that's you! %s\n", login_name);
}
