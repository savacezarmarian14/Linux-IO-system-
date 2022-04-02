#include "so_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BUFF_SIZE 4096

/* structura SO_FILE */

struct _so_file {
    int file_des; /* descriptorul fisierului (un index in FSB ul programului --procesului--) */
    char* pathname; /* calea catre fisier */
    char* mode; /* modurile de deschidere a fisierului */
    
    char* read_buffer; /* bufferul de citire */
    char* write_buffer; /* bufferul de scriere */
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
    
    /* aloc memorie pentru bufferele de i/o (buffering) */
    file->read_buffer   = (char*) malloc(BUFF_SIZE * sizeof(char)); /* citire */
    memset(file->read_buffer, '\0', sizeof(file->read_buffer)); /* initializez cu 0 */

    file-> write_buffer = (char*) malloc(BUFF_SIZE * sizeof(char));  /* scriere */
    memset(file->write_buffer, '\0', sizeof(file->write_buffer)); /* initializez cu 0 */
    
    int flags;

    if((strcmp(mode, "w") == 0) || (strcmp(mode, "w+") == 0) ||
        (strcmp(mode, "a") == 0) || (strcmp(mode, "a+") == 0)) /* dehide fisierul cu permisiuni */
        file->file_des = open(file->pathname, getFlags(file->mode), 0644);
    else /* deschide fisierul fara permisiuni */
        file->file_des = open(file->pathname, getFlags(file->mode)); 

    
    if(file->file_des == -1) 
        return NULL; /* returneaza NULL daca e eroare la deschidere */
    return file; /* returenaza SO_FILE *file in caz de succes */
    
    
}

/* Functia intoarce file_des al fisierului */

FUNC_DECL_PREFIX int so_fileno(SO_FILE *stream) {
    return stream->file_des;
}

/* functia care inchide fisierul */

FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream) {
    free(stream->pathname); /* eliberez memoria */
    free(stream->mode); /* eliberez memoria */
    free(stream->read_buffer); /* eliberez memoria bufferului de citire */
    free(stream->write_buffer); /* eliberez memoria bufferului de scriere */

    int close_result = close (stream->file_des); /* inchid file_des */
    if(close_result == 0) { /* in caz ca inchiderea fisierului a fost cu succes */
        free(stream); /* dezaloc memoria pt FILE */
        return 0;
    } else
        return SO_EOF; /* presupun ca e -1 */
}

/* Functia de citire a unui caracter (convertit la int) dintr un fisier */

FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream) {
    /* Cand citesc citesc din buffer un caracter doar daca are ceva in el, altfel, il populez */
    if(strlen(stream->read_buffer) == 0) {
        int rc = read(stream->file_des, stream->read_buffer, BUFF_SIZE);
        if(rc < 0)
            return SO_EOF;
    }
    
    unsigned char c = stream->read_buffer[0];
    strcpy(stream->read_buffer, stream->read_buffer + 1);
    
    return (int) c;

}

/* Functia de scriere a unui caracter (primit ca numar intreg) intr un fisier */

FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream) {
    /* bag caracterul in buffer -> scriu in fisier -> golesc buffer */ 
    stream->write_buffer[0] = (char) c;
    stream->write_buffer[1] = 0; /* l am bagat pe c in buffer */

    int wc = write(stream->file_des, stream->write_buffer, 1); /* scriu din buffer in fisier */

    memset(stream->write_buffer, 0, sizeof(stream->write_buffer)); /* golesc bufferul */

    if(wc < 0) {
        return SO_EOF; /* daca scrierea a esuat returnes EOF */
    }
    return c; /* retunrez caracterul (deja extins la int) */
}

FUNC_DECL_PREFIX
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    size_t total_bytes = nmemb * size;
    
    return nmemb;
}


FUNC_DECL_PREFIX
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    return 0; //TODO
}

FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream) {
    return 0; //TODO
}

FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence) {
    return 0; //TODO
}
FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream) {
    return 0; //TODO
}

FUNC_DECL_PREFIX int so_feof(SO_FILE *stream) {
    return 0; //TODO
}
FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream) {
    return 0; //TODO
}

FUNC_DECL_PREFIX SO_FILE *so_popen(const char *command, const char *type) {
    return NULL; //TODO
}
FUNC_DECL_PREFIX int so_pclose(SO_FILE *stream) {
    return 0; //TODO
}


int main(int argc, char* argv[]) {

    SO_FILE* inputFile = so_fopen("cez.txt", "r");
    printf("%d\n" , inputFile->file_des);
    char *s = malloc(100*sizeof(char));
    int rc = so_fgetc(inputFile);
    printf("%c\n", rc);
    rc = so_fgetc(inputFile);
        printf("%c\n", rc);

    so_fclose(inputFile); 
    

    return 0;
}
