%x PREFIX
%x PREFIX_OPT
%x PREFIX_HOST
%x PREFIX_USER
%x PARAMS
%x PARAM
%x TRAILING
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
servername	("*"|{shortname})(\.{shortname})+
middle		[^\:\0\r\n\ ][^\ \0\r\n]*
crlf 		(\r\n|\n\r)

%{
	#include "irc.h"
	#include <assert.h>
%}

%%

<INITIAL>{
	":"					BEGIN(PREFIX); return IRC_COLON;
	"PING"				BEGIN(PARAMS); return IRC_CMD_PING;
	"PONG"				BEGIN(PARAMS); return IRC_CMD_PONG;
	"QUIT"  			BEGIN(PARAMS); return IRC_CMD_QUIT;
	"PRIVMSG"			BEGIN(PARAMS); return IRC_CMD_PRIVMSG;
}

<PREFIX>{
	{servername}		BEGIN(PREFIX_OPT); return IRC_SERVERNAME;
	{nickname}			BEGIN(PREFIX_OPT); return IRC_NICKNAME;
}

<PREFIX_OPT>{
	"!"					BEGIN(PREFIX_USER); return IRC_EXCLAMATION;
	"@"					BEGIN(PREFIX_HOST); return IRC_AT;
	" "					BEGIN(INITIAL); return IRC_SPACE;
}

<PREFIX_USER>{user} 	BEGIN(PREFIX_OPT); return IRC_USER;
<PREFIX_HOST>{host} 	BEGIN(PREFIX_OPT); return IRC_HOST;

<PARAMS>{
	" "					BEGIN(PARAM); return IRC_SPACE;
	{crlf}				BEGIN(INITIAL); return IRC_CRLF;
}

<PARAM>{
	":"					BEGIN(TRAILING); return IRC_COLON;
	{middle}			BEGIN(PARAMS); return IRC_PARAM;
}

<TRAILING>{
	[^\0\n\r]*			BEGIN(PARAMS); return IRC_TRAILING;
}