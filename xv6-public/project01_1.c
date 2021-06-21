#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    int myPid = getpid();
    int myPPid = getppid();
    printf(1, "My pid is %d\n", myPid);
    printf(1, "My ppid is %d\n", myPPid);
    exit();
}