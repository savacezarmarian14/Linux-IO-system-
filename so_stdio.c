#include "so_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* structura SO_FILE */

struct _so_file {
    int filedes;
    char* pathname;
    char* mode;
};

/* functia care calculeaza flagurile pentru deschiderea fisierului */

int getFlags(const char* mode) {

    if(strcmp(mode, "r") == 0) {
        return O_RDONLY;
    } else if(strcmp(mode, "r+") == 0) {
        return O_RDWR;
    } else if(strcmp(mode, "w")  == 0) {
        return O_CREAT | O_TRUNC | O_WRONLY;
    } else if(strcmp(mode, "w+") == 0) {
        return O_CREAT | O_TRUNC | O_RDWR;
    } else if(strcmp(mode, "a")  == 0) {
        return O_CREAT | O_APPEND | O_WRONLY;
    } else if(strcmp(mode, "a+") == 0) {
        return O_CREAT | O_APPEND | O_RDWR;
    }

    return -1;
}
/* functia pentru deschiderea fisierului */

FUNC_DECL_PREFIX SO_FILE *so_fopen(const char* pathname, const char* mode) {
    SO_FILE* file = malloc(sizeof(SO_FILE));
    file->pathname = strdup(pathname);
    file->mode = strdup(mode);
    int flags;

    if((strcmp(mode, "w") == 0) || (strcmp(mode, "w+") == 0) ||
        (strcmp(mode, "a") == 0) || (strcmp(mode, "a+") == 0)) /* dehide fisierul cu permisiuni */
        file->filedes = open(file->pathname, getFlags(file->mode), 0644);
    else /* deschide fisierul fara permisiuni */
        file->filedes = open(file->pathname, getFlags(file->mode)); 

    
    if(file->filedes == -1) 
        return NULL; /* returneaza NULL daca e eroare la deschidere */
    return file; /* returenaza SO_FILE file in caz de succes */
    
    
}

/* functia care inchide fisierul */

FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream) {
    free(stream->pathname); /* eliberez memoria */
    free(stream->mode); /* eliberez memoria */
    int close_result = close (stream->filedes); /* inchid filedes */
    if(close_result == 0) { /* in caz ca inchiderea fisierului a fost cu succes */
        free(stream); /* dezaloc memoria pt FILE */
        return 0;
    } else
        return SO_EOF; /* presupun ca e -1 */
}

int main(int argc, char* argv[]) {

    SO_FILE* inputFile = so_fopen("cez.txt", "w+");
    int rc = write(inputFile->filedes, "cz", 2);
    if(rc == -1)
        printf("%s\n", strerror(errno));
    printf("%d\n" , inputFile->filedes);
    so_fclose(inputFile);
    

    return 0;
}