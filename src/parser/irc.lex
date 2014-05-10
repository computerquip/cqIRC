%x PREFIX
%x PREFIX_END
%x PREFIX_OPT
%x PREFIX_HOST
%x PREFIX_USER
%x PARAMS
%x PARAM
%x TRAILING
%x END

%option reentrant
%option case-insensitive
%option noyywrap
%option extra-type="struct cq_irc_message *"

special		[\x5B-\x60\x7B-\x7D]
hostchar	("_"|"/"|[[:alnum:]])

user 		[^\0\r\n\x20\@]+
nickname	([[:alpha:]]|{special})([[:alnum:]]|{special}|"-")*{0,8}
shortname	[[:alnum:]]([[:alnum:]]|"-")*[[:alnum:]]*
host 		{hostchar}({hostchar}|"-"|"."|":")*
hostname	{shortname}(\.{shortname})*
servername	{hostname}
middle		[^\:\0\r\n\ ][^\ \0\r\n]*
crlf 		(\r\n|\n\r)

%{
	#include "irc-types.h"
	#include <assert.h>
%}

%%

<INITIAL>{
	":"					BEGIN(PREFIX);
	"PING"				BEGIN(PARAMS); 
	"PONG"				BEGIN(PARAMS); 
	"QUIT"  			BEGIN(PARAMS); 
	"PRIVMSG"			BEGIN(PARAMS); 
	"NOTICE"			BEGIN(PARAMS); 
	"ERROR"				BEGIN(PARAMS);
	.					return -1; /* Unknown Command */
}

<PREFIX>{
	{nickname} 			BEGIN(PREFIX_OPT); yyextra->prefix.source = strndup(yytext, yyleng);
	{servername}		BEGIN(PREFIX_END); yyextra->prefix.source = strndup(yytext, yyleng); 
}

<PREFIX_END>{
	" " 				BEGIN(INITIAL);
}

<PREFIX_OPT>{
	"!"					BEGIN(PREFIX_USER);
	"@"					BEGIN(PREFIX_HOST);
	" "					BEGIN(INITIAL);
}

<PREFIX_USER>{user} 	BEGIN(PREFIX_OPT); yyextra->prefix.user = strndup(yytext, yyleng);
<PREFIX_HOST>{host} 	BEGIN(PREFIX_OPT); yyextra->prefix.host = strndup(yytext, yyleng);

<PARAMS>{
	" "					BEGIN(PARAM);
	{crlf}				BEGIN(END);
}

<END>{
	<<EOF>>				return 0;
}

<PARAM>{
	":"					BEGIN(TRAILING);
	{middle}			BEGIN(PARAMS); yyextra->params.param[yyextra->params.length] = strndup(yytext, yyleng); ++yyextra->params.length;
}

<TRAILING>{
	[^\0\n\r]*			BEGIN(PARAMS); yyextra->trailing = strndup(yytext, yyleng);
}

<<EOF>>					return -2;  /* Not enough input */

%%
