/*
implement cat  cmd in C
reverse file content
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* pentru open(), exit() */
#include <fcntl.h> /* O_RDWR */
#include <errno.h> /* perror() */
#define MAX_LINES 10

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

    file = open("fileTac" , O_RDONLY);
    if(file < 0)
        fatal("nu s-a putut deschide fisierul");
 
    // cautam initial fiecare sfarsit de linie
    // si l retinem intr un vector
    int *endLines = calloc(MAX_LINES,sizeof(int));
    int lines = 0;
    lseek(file, 0, SEEK_SET);
    cat = read(file, buf,sizeof(buf));
    buf[cat] = '\0';
    for(int i = 0 ;i < strlen(buf); i++){
        if(buf[i] == '\n')
            endLines[lines++] = i;
    }
    
    //am gasit fiecare sfarsit de linie 
    //acum trebuie sa ne folosim de lseek pentru a citi
    //fiecare linie incepand cu ultima 
    for(int i = lines-1 ;i >= 1; i--){
        lseek(file,endLines[i-1]+1,SEEK_SET);
        //fac diferenta pentru a vedea cate caractere 
        //sunt pe fiecare linie
        cat = read(file, buf,endLines[i]- endLines[i-1] );
        write(1, buf, cat);
    }
    //afisam si prima linei
    lseek(file,0,SEEK_SET);
    cat = read(file, buf,endLines[0]);
    write(1,buf,cat);
    close(file);
    free(endLines);
    return 0;
}