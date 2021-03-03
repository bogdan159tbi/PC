/*
implement cat  cmd in C
view file content
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* pentru open(), exit() */
#include <fcntl.h> /* O_RDWR */
#include <errno.h> /* perror() */

void fatal(char * mesaj_eroare)
{
    perror(mesaj_eroare);
    exit(0);
}

int main(void)
{
    int file;
    int cat;
    char buf[1024];

    file = open("content" , O_RDONLY);
    if(file < 0)
        fatal("nu s-a putut deschide fisierul");

    lseek(file, 0, SEEK_SET);

    while ((cat = read(file, buf, sizeof(buf)))) {
        if (cat < 0)
            fatal("Eroare la citire");
        write(1,buf,cat);
    }

    close(file);
    return 0;
}