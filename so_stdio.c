#include "so_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define BUFF_SIZE 4096
#define PIPE_READ 0
#define PIPE_WRITE 1

/* structura SO_FILE */

struct _so_file {
    int file_des; /* descriptorul fisierului (un index in FSB ul programului --procesului--) */
    char *pathname; /* calea catre fisier */
    char *mode; /* modurile de deschidere a fisierului */

    unsigned char *read_buffer;  /* bufferul de citire */
    int read_count; /* cate elemente am in buffer */

    unsigned char *write_buffer; /* bufferul de scriere */
    int write_count; /* cate elemente am in buffer */
    
    short int last_operation; /* 0-citire 1-scriere */
    short int err;
    
    int is_pipe;
    pid_t child_pid;
    off_t cursor_pos;
};

/* functia care calculeaza flagurile pentru deschiderea fisierului */

int getFlags(const char* mode) {
    if(strcmp(mode, "r") == 0) 
        return O_RDONLY;
    else if(strcmp(mode, "r+") == 0)
        return O_RDWR;
    else if(strcmp(mode, "w")  == 0)
        return (O_CREAT | O_TRUNC | O_WRONLY);
    else if(strcmp(mode, "w+") == 0)
        return (O_RDWR | O_CREAT | O_TRUNC);
    else if(strcmp(mode, "a")  == 0)
        return (O_WRONLY | O_CREAT | O_APPEND);
    else if(strcmp(mode, "a+") == 0)
        return (O_RDWR | O_CREAT | O_APPEND);
    return -1;
    }

/* functia pentru deschiderea fisierului */

FUNC_DECL_PREFIX SO_FILE *so_fopen(const char* pathname, const char* mode) 
{
    SO_FILE* file = (SO_FILE *) malloc(sizeof(SO_FILE));
    file->pathname = strdup(pathname);
    file->mode = strdup(mode);
    file->last_operation = 0; 
    file->cursor_pos = 0;
    file->err = 0;
    file->is_pipe = 0;
    
    /* aloc memorie pentru bufferele de i/o (buffering) */
    file->read_buffer   = (char*) malloc(BUFF_SIZE * sizeof(unsigned char)); /* citire */
    if(!file->read_buffer)
        return NULL;
    memset(file->read_buffer, '\0', sizeof(file->read_buffer)); /* initializez cu 0 */

    file-> write_buffer = (char*) malloc(BUFF_SIZE * sizeof(unsigned char));  /* scriere */
    if(!file->write_buffer)
        return NULL;
    memset(file->write_buffer, '\0', sizeof(file->write_buffer)); /* initializez cu 0 */
    
    if((strcmp(mode, "w") == 0) || (strcmp(mode, "w+") == 0) ||
        (strcmp(mode, "a") == 0) || (strcmp(mode, "a+") == 0)) /* dehide fisierul cu permisiuni */
        file->file_des = open(file->pathname, getFlags(file->mode), 0644);
    else /* deschide fisierul fara permisiuni */
        file->file_des = open(file->pathname, getFlags(file->mode)); 

    file->read_count = 0;  /* setez la 0 numarul de elemente din bufferul de citire */
    file->write_count = 0; /* setes la 0 numarul de elemente din bufferul de scriere */
    if(file->file_des == -1) {
        so_fclose(file); //eliberez memoria alocata 
        return NULL; /* returneaza NULL daca e eroare la deschidere */
    }
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
    
    if(stream->write_count != 0) {
        int flush = so_fflush(stream);
        if(flush < 0) {
            free(stream->write_buffer);
            close(stream->file_des);
            free(stream);
            return SO_EOF;
        }
    }
    free(stream->write_buffer); /* eliberez memoria bufferului de scriere */

    int close_result = close (stream->file_des); /* inchid file_des */
    free(stream); /* dezaloc memoria pt FILE */

    if(close_result == 0) { /* in caz ca inchiderea fisierului a fost cu succes */
        return 0;
    } else
        return SO_EOF; /* presupun ca e -1 */
}

/* Functia de citire a unui caracter (convertit la int) dintr un fisier */


FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream) {
    static int index = 0;
    if(stream->read_count == 0)
        index = 0;
    /* Citesc din buffer un caracter doar daca are ceva in el, altfel, il populez */
    if(BUFF_SIZE - index == 0) {
        index = 0;
        stream->read_count = 0;
        memset(stream->read_buffer, '\0', BUFF_SIZE);
    }

    if(stream->read_count == index) {
        int rc = read(stream->file_des, stream->read_buffer + index, BUFF_SIZE - index);
        if(rc <= 0) {
            lseek(stream->file_des, 0, SEEK_SET);
            if(rc != 0)
                stream->err = 1;
            return SO_EOF;
        }
        stream->read_count += rc;
    }
    unsigned int c = (unsigned int) stream->read_buffer[index]; /* nici mie nu mi place dar asa merge treaba... */
    index ++;
    stream->last_operation = 0; /* ultima operatie -> citire */ 
    return c;

}

/* Functia de scriere a unui caracter (primit ca numar intreg) intr un fisier */

FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream) {
    static int index = 0;
    if(stream->write_count == 0) 
        index = 0;
    /* bag caracterul in buffer -> scriu in fisier -> golesc buffer */
    if(stream->write_count == BUFF_SIZE) {
        int flush = so_fflush(stream);
        index = 0;
        if(flush == SO_EOF) {
            stream->err = 1;
            return SO_EOF;
        }
    }

    
    
    stream->write_buffer[index] = (unsigned char) c;
    stream->write_count ++;
    stream->last_operation = 1; /* ultima operatie -> scriere */
    index++;

    
    /* 
     *nu fac scrierea direct in fisier deoarece o fac la final cand inchid fisierul 
     *la apelarea so_fflush sau cand SO_FILE->write_buffer a atins capacitatea maxima 
     */
    
    unsigned int aux_c = (unsigned int) c;
    return aux_c; /* retunrez caracterul (deja extins la int) */
}

FUNC_DECL_PREFIX
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    int reads = 0;
    for(int i = 0; i < nmemb; i++) {
        for(int j = 0; j < size; j++) {
        
            int rc = so_fgetc(stream);
            if(rc < 0) {
                stream->is_pipe = 0;
                return 0;
            }
            if(rc == 0 && stream->is_pipe == 1)
                stream->is_pipe = 0;
            *(char *)ptr = (char)rc;

            ptr++;
        }
        reads++;
    }
    stream->cursor_pos = (off_t) nmemb;
    return reads;
}


FUNC_DECL_PREFIX
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    const unsigned char* aux_ptr = ptr;
    for(int i = 0; i < nmemb ; i++) {
        for(int j = 0; j < size ; j++) {
            unsigned char c = aux_ptr[i * size + j];
            unsigned int wc = (unsigned int)so_fputc((unsigned int)c, stream);
            if(wc == SO_EOF)
                return 0;
        }
    }
    stream->cursor_pos = (off_t) nmemb;
    return nmemb;
}

FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream) {
    int buff_len = stream->write_count;
    int bytes_writen = 0;
    if(stream->write_count < BUFF_SIZE){
        stream->write_buffer[buff_len] = '\0';
    }
    while(buff_len > bytes_writen) {
        int wc = write(stream->file_des, 
            stream->write_buffer,
            buff_len - bytes_writen);
        if(wc < 0)
            return SO_EOF;
        bytes_writen += wc;
    }
    
    memset(stream->write_buffer, '\0', BUFF_SIZE); /* setez cu 0 */
    stream->write_count = 0;
    return 0;
}

FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence) {
    off_t so_offset = (off_t) offset;
    off_t ret = lseek(stream->file_des, so_offset, whence);
    stream->cursor_pos = ret;
    if(stream->last_operation == 0) {
        /* CITIRE */
        stream->read_count = 0;
        memset(stream->read_buffer, '\0', BUFF_SIZE);
    } else if (stream->last_operation == 1) {
        /* SCRIERE */
        so_fflush(stream);
    }
    if(ret != -1)
        return 0;
    return -1;
}
FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream) {
    return stream->cursor_pos;
}

FUNC_DECL_PREFIX int so_feof(SO_FILE *stream) {
    static int pipe_calls = 0;
    if(stream->is_pipe) {
        pipe_calls ++;
    }
    if(pipe_calls > 1 && stream->is_pipe == 0) {
        return 1;
    }
    off_t old_pos = lseek(stream->file_des, 0, SEEK_CUR);
    off_t end_pos = lseek(stream->file_des, 0, SEEK_END);
    if(stream->is_pipe == 1)
        return 0;
    
    if(old_pos == end_pos) {
        return 0;
    }
    lseek(stream->file_des, old_pos, SEEK_SET);
    return 1;
}
FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream) {
    if(stream->err == 1)
        return 1; 
    return 0;
}
SO_FILE* alloc_so_file() {
    SO_FILE* stream = (SO_FILE*) malloc(sizeof(SO_FILE));
    stream->is_pipe = 1;

    stream->read_buffer = (unsigned char*) malloc(BUFF_SIZE * sizeof(unsigned char));
    if(!stream->read_buffer)
        return NULL;
    stream->read_count = 0;

    stream->write_buffer = (unsigned char*) malloc(BUFF_SIZE * sizeof(unsigned char));
    if(!stream->write_buffer)
        return NULL;
    stream->write_count = 0;

    return stream;
}
FUNC_DECL_PREFIX SO_FILE *so_popen(const char *command, const char *type) {
    int fds[2];
    int rc = pipe(fds);
    SO_FILE* stream = alloc_so_file();  

    pid_t pid = fork();
    if(pid == 0) {
        /* copil */
        if(type[0] == 'r') {
            dup2(fds[PIPE_WRITE], STDOUT_FILENO);
            close(fds[PIPE_WRITE]);
            close(fds[PIPE_READ]);
        } else if(type[0] == 'w') {
            dup2(fds[PIPE_READ], STDIN_FILENO);
            close(fds[PIPE_READ]);
            close(fds[PIPE_WRITE]);
        }
        char* const argv[] = {"sh", "-c", (char*) command, NULL};
        execv("/bin/sh", argv);

    } else if (pid > 0) {
        /* parinte */
        stream->child_pid = pid;
        if(type[0] == 'r') {
            stream->file_des = fds[PIPE_READ];
            close(fds[PIPE_WRITE]);
        } else if(type[0] == 'w') {
            stream->file_des = fds[PIPE_WRITE];
            close(fds[PIPE_READ]);
        }
        return stream;

    } else if(pid == -1){
        close(fds[PIPE_READ]);
        close(fds[PIPE_WRITE]);
        free(stream->write_buffer);
        free(stream->read_buffer);
        free(stream);
        return NULL; 
    }
}
FUNC_DECL_PREFIX int so_pclose(SO_FILE *stream) {
    int status = 0;
    if(stream->write_count != 0)
        so_fflush(stream);
    int close_res = close(stream->file_des);
    
    int result = waitpid(stream->child_pid, &status, 0);
    
    free(stream->write_buffer);
    free(stream->read_buffer);
    free(stream);
    if(result == -1)
        return -1;
    if(close_res == -1)
        return -1;
    int es = WEXITSTATUS(status);

    return es;
}


