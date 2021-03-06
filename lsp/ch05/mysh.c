#define MYSH_PROMPT "mysh> "

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>

// --------------------------------------------------------------------------
// these fctns are defined in other source files in this directory.
int cat(int argc, char** argv);
int wc(int argc, char** argv);
int ls(int argc, char** argv);

// --------------------------------------------------------------------------
void do_cmd(char* buf, int len);
int parse_cmd(char* buf, char** vbuf, int argcount);
int is_built_in_cmd(int argc, char** argv);
int exec_cmd(char** argv);
int print_wait_status(FILE* wfp, int pid, int st);

// --------------------------------------------------------------------------
int g_terminate;
int g_timeout;

// --------------------------------------------------------------------------
int print_wait_status(FILE* wfp, int pid, int st)
{
    fprintf(wfp, "\n");

    fprintf(wfp,"waitpid() returned: %6d",pid);

    if (WIFEXITED(st)) {
        fprintf(wfp,"exit: %3d\n", WEXITSTATUS(st));
    } else if (WIFSTOPPED(st)) {
        fprintf(wfp,"stop status: %3d\n", WSTOPSIG(st));
    } else if (WIFSIGNALED(st)) {
        fprintf(wfp,"termination signal: %3d\n", WTERMSIG(st));

        // fprintf(wfp,"\tcore dump: %s\n", WIFCORE(st) ? "yes" : "no");
    }

    return 0;
}


// --------------------------------------------------------------------------
// returns: non-zero if this argv is a built-in command.
int is_built_in_cmd(int argc, char** argv)
{
    int rc = 0;
    int ec;

    if ((0 == strncmp(argv[0], "exit", 4)) || (0 == strncmp(argv[0], "quit", 4))) {
        rc = 1; // yes, this was a built-in.
        g_terminate = 1;
    } else if (0 == strncmp(argv[0], "cd", 2)) {
        // change directory.
        rc = 1;
        if (argv[1] && (0 != (ec = chdir(argv[1])))) {
            fprintf(stderr, "failed to change directory to '%s'\n", argv[1]);
            rc = -1;
        }
    } else if (0 == strncmp(argv[0], "ls", 2)) {
        // list the directory
        ec = ls(argc, argv);
        rc = 1;
    } else if (0 == strncmp(argv[0], "cat", 3)) {
        // cat the files named on the cmd line
        ec = cat(argc, argv);
        rc = 1;
    } else if (0 == strncmp(argv[0], "wc", 2)) {
        // count the chars, words and lines
        ec = wc(argc, argv);
        rc = 1;
    } else if (0 == strncmp(argv[0], "hello", 5)) {
        // give a friendly greeting.
        fprintf(stderr, "\nHello! from process %d\n", getpid());
        rc = 1;
    } else {
        // this is not an internal command.
        rc = 0;
    }

    return rc;
}


// --------------------------------------------------------------------------
int exec_cmd(char** argv)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork() returned: %d\n", pid);
        exit(-1);
    } else if (0 == pid) {
        // child process
        int ec;
        ec = execvp(argv[0], argv);
        if (ec < 0) {
            fprintf(stderr, "execvp(%s) returned: %d\n", argv[0], ec);
            _exit(errno);  // terminate the child
        }
    } else {
        // parent process
        int st;
        waitpid(pid, &st, 0);
        // print_wait_status(stdout, pid, st);
    }
    return 0;
}


// --------------------------------------------------------------------------
int parse_cmd(char* buf, char** vbuf, int n)
{
    int i = 0;
    const char* delim = " ,\t\n";
    char* token;
    size_t len;

    token = strtok(buf, delim);
    while (token && i < n) {
        len = strlen(token);
        vbuf[i] = malloc(len + 1);
        strcpy(vbuf[i], token); // happily, strcpy() copies the terminating zero too.
        token = strtok(NULL, delim);
        ++i;
    }

    vbuf[i] = 0; // it should already be the case.

    return i;
}


// --------------------------------------------------------------------------
void do_cmd(char* buf, int bufsize)
{
    int i;
    char* vbuf[32];
    int arg_count;
    int max_arg_count = sizeof vbuf / sizeof vbuf[0];

    memset(vbuf, 0, sizeof vbuf);

    arg_count = parse_cmd(buf, vbuf, max_arg_count - 1);
    //printf("(%s:%d) %s(),  arg_count: %d\n", __FILE__, __LINE__, __FUNCTION__, arg_count);

    if (arg_count < 1) {
        ; // nothing to do.
    } else if (is_built_in_cmd(arg_count, vbuf)) {
        ; // built in cmds are handled.
    } else {
        // an external command
        exec_cmd(vbuf);
    }

    // free the arg list
    for (i=0; i<arg_count; ++i) {
        if (vbuf[i]) {
            free(vbuf[i]);
        }
    }
}

// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    char* prompt;
    int fd;
    char* buf;
    int bufsize;
    int nr_read;
    struct pollfd ps;
    int poll_rc;

    g_timeout = 15000; // fifteen seconds.

    // allcocate an input buffer.
    bufsize = 4096;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }

    fd = 0; // we're reading from stdin.

    if (isatty(fd)) {
        prompt = MYSH_PROMPT;
        //fprintf(stderr, "%s", prompt);
    } else {
        // if not a terminal...do not show a prompt.
        prompt = "";
    }

    // set up our poll struct.
    ps.fd = fd;
    ps.events = POLLIN;
    ps.revents = 0;


    while (!g_terminate) {
        if (prompt) {
            fprintf(stderr, "%s", prompt);
        }

        // wait a while for input to be ready.
        poll_rc = poll(&ps, 1, g_timeout);

        if (0 == poll_rc) {
            // timed out... ???
            fprintf(stderr, "poll() timed out\n");
            continue;
        } else if (poll_rc < 0) {
            // an error.
            perror("poll()");
            break;
        } // else
        // input fd is ready to be read.

        memset(buf, 0, sizeof buf);
        nr_read = read(fd, buf, bufsize-1);

        if ((0 < nr_read) && *buf) {
            do_cmd(buf, bufsize);
        }

    } // while

out:
    if (buf) {
        free(buf);
    }

    return 0;
}
