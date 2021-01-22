#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    if ( argc != 2 ) {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
    int filefd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    assert ( filefd > 0 );

    int pipefd_stdout[2];
    int ret = pipe(pipefd_stdout);
    assert(ret != -1);

    int pipe_file[2];
    ret = pipe(pipe_file);
    assert(ret != -1);

    // splice: std_in redicted to pipefd_stdout
    ret = splice(STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32768,
            SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);
    
    // copy pipefd_stdout to pipe_file
    ret = tee(pipefd_stdout[0], pipe_file[1], 32768, SPLICE_F_NONBLOCK);
    assert(ret != -1);

    // pipe_file redirect to filefd
    ret = splice(pipe_file[0], NULL, filefd, NULL, 32768,
            SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    //  pipefd_stdout redirect to stdout
    ret = splice(pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 32768,
            SPLICE_F_MORE | SPLICE_F_MORE);
    assert(ret != -1);

    close(filefd);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);
    close(pipe_file[0]);
    close(pipe_file[1]);
    return 0;
}
