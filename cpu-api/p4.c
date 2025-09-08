#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

int
main(int argc, char *argv[])
{
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n"); // for failed, exit
        exit(1);
    } else if (rc == 0) {
        // child (new process): redirect stdout to a file
        close(STDOUT_FILENO);
        open("./p4.output", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

        // now exec "wc"...
        char *myargs[3];
        myargs[0] = strdup("wc"); // program: "wc" (word count)
        myargs[1] = strdup("p4"); // argument: file to count -> use exec file
        myargs[2] = NULL; // marks end of array
        execvp(myargs[0], myargs); // runs word count
    }
    else
    {
        int wc = wait(NULL);
    }
    return 0;
}
