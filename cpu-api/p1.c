// fork()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    printf("Hello World (pid:%d)\n", (int)getpid());
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n"); // for failed, exit
        exit(1);
    } else if (rc == 0) {
        printf("Hello, I am child (pid:%d)\n", (int)getpid()); // child (new process)
    } else {
        printf("Hello, I am parent of %d (pid:%d)\n", rc, (int)getpid()); // parent goes down this
    }
    return 0;
}

/**
 * parent (pid:13381)  |  child process (pid:13388)
 * rc = 13388          |  rc = 0
 * 
 * Both processes running, but random order. -> CPU scheduler
 */
