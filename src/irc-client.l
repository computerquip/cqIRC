%x GENERIC_INITIAL
%x GENERIC_PARAMS
%x PREFIX
%x PREFIX_OPT
%x PARAMS
%x PARAM
%x TRAILING

%option stack
%option reentrant
%option noyywrap
%option nounput
%option noinput
%option extra-type="struct cq_irc_session *"

special		[\x5B-\x60\x7B-\x7D]
hostchar	("_"|"/"|[[:alnum:]])

command 	[[:alpha:]]+|[[:digit:]]{3}
user 		[^\0\r\n\x20\@]+
nickname	([[:alpha:]]|{special})([[:alnum:]]|{special}|"-")*{0,8}
shortname	[[:alnum:]]([[:alnum:]]|"-")*[[:alnum:]]*
host 		{hostchar}({hostchar}|"-"|"."|":")*
hostname	{shortname}(\.{shortname})*
servername	{hostname}
middle		[^\:\0\r\n\ ][^\ \0\r\n]*
crlf 		(\r\n|\n\r)

%{
	#include "irc-client-internal.h++"
	#include <assert.h>
	#include <string.h>

	#define IRC_EVENT_TEST(name) \
		do { \
			if (yyextra->callbacks.signal_##name) \
				event_signal = yyextra->callbacks.signal_##name; \
			else { \
				destroy_message(&message); \
				return 1; \
			} \
		} while(0) 

	#define IRC_EVENT_TEST_EXTRA(name, extra) \
		do { \
			if (yyextra->callbacks.signal_##name) { \
				extra_event_signal = yyextra->callbacks.signal_##name; \
				command = (extra); \
			} \
			else { \
				destroy_message(&message); \
				free(extra); \
				return 1; \
			} \
		} while(0) \

	#define IRC_ADD_PARAM(X) \
		do { message.params.param[message.params.length] = (X); ++message.params.length; } while(0)

	static void destroy_message(cq_irc_message* msg)
	{
		free(msg->prefix.host);
		free(msg->prefix.source);
		free(msg->prefix.user);
	
		for (int j = 0; j < msg->params.length; ++j) {
			free(msg->params.param[j]);
		}

		free(msg->trailing);
	}
%}

%%
%{
	void (*event_signal)(struct cq_irc_session*, struct cq_irc_message*);
	void (*extra_event_signal)(struct cq_irc_session*, const char* command, struct cq_irc_message*);
	
	char *command = NULL; /* We only set this if we don't determine what event it is so the user can figure it out himself. */
	struct cq_irc_message message = { 0 };

	if (yyextra->use_generic == true)
		yy_push_state(GENERIC_INITIAL, yyscanner);
%}

<INITIAL>{
	":"			yy_push_state(PREFIX, yyscanner);
	(?i:"004")		yy_push_state(PARAMS, yyscanner); IRC_EVENT_TEST(welcome);
	(?i:"PING")		yy_push_state(PARAMS, yyscanner); IRC_EVENT_TEST(ping);
	(?i:"PRIVMSG")		yy_push_state(PARAMS, yyscanner); IRC_EVENT_TEST(privmsg);
	(?i:"NOTICE")		yy_push_state(PARAMS, yyscanner); IRC_EVENT_TEST(notice);
	(?i:"ERROR")		yy_push_state(PARAMS, yyscanner); IRC_EVENT_TEST(error);
	{command}		yy_push_state(GENERIC_PARAMS, yyscanner); IRC_EVENT_TEST_EXTRA(unknown, strndup(yytext, yyleng));
}

<GENERIC_INITIAL>{
	":"			yy_push_state(PREFIX, yyscanner);
	{command}		yy_push_state(GENERIC_PARAMS, yyscanner); IRC_EVENT_TEST_EXTRA(unknown, strndup(yytext, yyleng));
}

<PREFIX>{
	{nickname}|{servername} yy_push_state(PREFIX_OPT, yyscanner); message.prefix.source = strndup(yytext, yyleng);
}

<PREFIX_OPT>{
	"!{user}"		message.prefix.user = strndup(yytext + 1, yyleng - 1);
	"@{host}"		message.prefix.host = strndup(yytext + 1, yyleng - 1);
	" "			yy_pop_state(yyscanner); yy_pop_state(yyscanner);
}

<GENERIC_PARAMS>{
	" "			yy_push_state(PARAM, yyscanner);
	{crlf}			extra_event_signal(yyextra, command, &message); destroy_message(&message); free(command); return 0;
}

<PARAMS>{
	" "			yy_push_state(PARAM, yyscanner);
	{crlf}			event_signal(yyextra, &message); destroy_message(&message); return 0;
}

<PARAM>{
	":"			yy_push_state(TRAILING, yyscanner);
	{middle}		yy_pop_state(yyscanner); IRC_ADD_PARAM(strndup(yytext, yyleng));
}

<TRAILING>{
	[^\0\n\r]*		yy_pop_state(yyscanner); yy_pop_state(yyscanner); message.trailing = strndup(yytext, yyleng);
}

<<EOF>>				destroy_message(&message); free(command); return -2;  /* Not enough input */
