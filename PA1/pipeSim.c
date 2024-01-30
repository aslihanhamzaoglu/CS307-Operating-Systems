#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

int main (int argc, char *argv[] ) {

    printf("I’m SHELL process, with PID:%d - Main command is: man df | grep -A 1 '\\-a' > output.txt\n", (int) getpid());

    int fd[2]; // pipe to communicate between MAN and GREP
    pipe(fd);

	int rc = fork(); // Child for MAN

	if (rc < 0) {

		fprintf(stderr, "fork failed\n");
		exit(1);
	}
	else if (rc == 0) { // MAN

        printf("I’m MAN process, with PID:%d - My command is: man df\n", (int) getpid());

        int rc2 = fork(); //child for Grep

        if (rc2 < 0) {
            fprintf(stderr, "fork failed\n");
            exit(1);
        }
        else if (rc2 == 0 ) { //GREP

            printf("I’m GREP process, with PID:%d - My command is: grep -A 1 '\\-a'\n", (int) getpid());

            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);

            int output = open("output.txt", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            dup2(output, STDOUT_FILENO );

            char *grep_c[] = { "grep","-A 1","\\-a", NULL };
            execvp(grep_c[0], grep_c);

        }
        else{ //MAN
        
            dup2(fd[1], STDOUT_FILENO);

            char *command[] = { "man", "df", NULL };
            execvp(command[0], command);
        }
	}
    wait(NULL);
    wait(NULL);

    printf("I'm SHELL process, with PID:%d - execution is completed, you can find the results in output.txt\n", (int) getpid());

    return 0;
}