// wait()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
        int wc = wait(NULL);
        printf("Hello, I am parent of %d (wc:%d) (pid:%d)\n", rc, wc, (int)getpid()); // parent goes down this
    }
    return 0;
}

/**
 * from p1, we can see parent and child processes are running, but we don't know the specific order.
 * so this is why we can use wait() for waiting child prcoesses running, then parent running in the end.
 */
