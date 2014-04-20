%x PREFIX
%x PREFIX_OPT
%x PREFIX_HOST
%x PREFIX_USER
%x PARAMS
%x PARAM
%x TRAILING
%x PRE_COMMAND

%option reentrant
%option noyywrap
%option case-insensitive

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
	#include "irc.h"
	#include <assert.h>

	#define YY_DECL int yylex(yyscan_t yyscanner, char** data)
%}

%%

<INITIAL>{
	":"					BEGIN(PREFIX);
	"PING"				BEGIN(PARAMS); return IRC_TOKEN_CMD_PING;
	"PONG"				BEGIN(PARAMS); return IRC_TOKEN_CMD_PONG;
	"QUIT"  			BEGIN(PARAMS); return IRC_TOKEN_CMD_QUIT;
	"PRIVMSG"			BEGIN(PARAMS); return IRC_TOKEN_CMD_PRIVMSG;
}

<PREFIX>{
	{nickname} 			BEGIN(PREFIX_OPT); *data = strndup(yytext, yyleng); return IRC_TOKEN_SOURCE;
	{servername}		BEGIN(PRE_COMMAND); *data = strndup(yytext, yyleng); return IRC_TOKEN_SOURCE;
}

<PRE_COMMAND>{
	" " 				BEGIN(INITIAL);
}

<PREFIX_OPT>{
	"!"					BEGIN(PREFIX_USER);
	"@"					BEGIN(PREFIX_HOST);
	" "					BEGIN(INITIAL);
}

<PREFIX_USER>{user} 	BEGIN(PREFIX_OPT); *data = strndup(yytext, yyleng); return IRC_TOKEN_USER;
<PREFIX_HOST>{host} 	BEGIN(PREFIX_OPT); *data = strndup(yytext, yyleng); return IRC_TOKEN_HOST;

<PARAMS>{
	" "					BEGIN(PARAM);
	{crlf}				BEGIN(INITIAL); return IRC_TOKEN_CRLF;
}

<PARAM>{
	":"					BEGIN(TRAILING);
	{middle}			BEGIN(PARAMS); *data = strndup(yytext, yyleng); return IRC_TOKEN_PARAM;
}

<TRAILING>{
	[^\0\n\r]*			BEGIN(PARAMS); *data = strndup(yytext, yyleng); return IRC_TOKEN_TRAILING;
}