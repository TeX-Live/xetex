/*2:*/
#line 74 "extconv.w"

#include <stdio.h>
#include <stdlib.h>


int main(argc,argv)
int argc;
char*argv[];

{int ch;


fprintf(stdout,"\\def\\CJKpreproc{}");

ch= fgetc(stdin);

while(!feof(stdin))
{if(ch>=0x81&&ch<=0xFE)
{fputc(ch,stdout);

ch= fgetc(stdin);
if(!feof(stdin))
fprintf(stdout,"%d\177",ch);
}
else
fputc(ch,stdout);

ch= fgetc(stdin);
}
exit(EXIT_SUCCESS);
return 0;
}/*:2*/
