// exec()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
        char *myargs[3];
        myargs[0] = strdup("wc"); // program: "wc" (word count)
        myargs[1] = strdup("p3"); // argument: file to count -> use exec file
        myargs[2] = NULL; // marks end of array
        execvp(myargs[0], myargs); // runs word count
        printf("This shouldn't print out");
    }
    else
    {
        int wc = wait(NULL);
        printf("Hello, I am parent of %d (wc:%d) (pid:%d)\n", rc, wc, (int)getpid());// parent goes down this
    }
    return 0;
}

/**
 *  Executing task: gcc -o p3 /Users/zhangminxin/Desktop/OSTEP/cpu-api/p3.c && ./p3
 * Hello World (pid:18858)
 * Hello, I am child (pid:18869)
 *      8      70   33888 p3
 * Hello, I am parent of 18869 (wc:18869) (pid:18858)
 *
 * It actually not create a new process, just replaced the current running one as new one(wc)
 * fork() -> create a new one -> 1->2
 * exec() -> replace the existing one -> 1->1 -> PID is the same, but program changes
 */
