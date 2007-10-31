/* lex.l - lexical analyzer for rail program */

%{

#include <string.h>

#include "rail.h"
#include "gram.h"

int copy;	/* copy to output flag */

#define	COPY	(copy && fputs(yytext,outf))

extern YYSTYPE yylval;

%}

%option yylineno

%%

[a-zA-Z_][0-9a-zA-Z_.]* {
		COPY;
		yylval.id=lookup(yytext);
		return(IDENTIFIER);
}

[0-9][0-9]* {
		COPY;
		yylval.num=atoi(yytext);
		return(NUMBER);
}

\\rail@i {
		COPY;
		return(RAILI);
}

\\rail@p {
		COPY;
		return(RAILP);
}

\\rail@t {
		COPY;
		return(RAILT);
}

\\par {
		COPY;
}

\\\\ {		COPY;
		return(RAILCR);
}

\\[a-zA-Z@]+ {
		COPY;
		return(CS);
}

\\[^ \n\r\t] {
		COPY;
		return(CS);
}

'[^\n\r\t']*' {
		COPY;
		yytext[yyleng-1]='\0';
		yylval.text=mcheck(strdup(yytext+1));
		return(STRING);
}

\[[^\n\r\t\]]*\] {
		COPY;
		yytext[yyleng-1]='\0';
		yylval.text=mcheck(strdup(yytext+1));
		return(ANNOT);
}

\"[^\n\r\t\"]*\" {
		COPY;
		yytext[yyleng-1]='\0';
		yylval.text=mcheck(strdup(yytext+1));
		return(STRING);
}

[ \n\r\t]+ {
		COPY;
}

. {
		COPY;
		return(yytext[0]);
}

