#include <stdio.h>

#define YY_DECL int yylex(yyscan_t yyscanner, char** data)
#define TEST_2

#include "../irc-types.h"
#include "../irc.lex.h"
#include "../irc.h"

const char *test_strings[] = {
	":dala!~lol@ack.com PONG\r\n"
	,":skorgon!~skorgon@gateway/tor-sasl/skorgon QUIT :Remote host closed the connection\r\n"
	,":LOLOL!~wtf@BOOTY/CALL PING 23423424\r\n"
	,"PRIVMSG #cplusplus :hdsahfiqiu40-8325783204989-294io3j45uoi2549849-9i4u95o-4320jru35trjjoj54hn6ji54i-=59423iu58342-9=8iu29342iurutrjfnmnm 2eo0934UOIJHY8aut yu&i^*&ui&^*#iiuyeio\r\n"
	,"QUIT a d f as awsfae 1 3 3 4 34 343 234 234 :324234234234234asfd  sd asfd\r\n"
};

const char *token2string_table[] = {
	 ""/* Nothing */
	,"CRLF"
	,"COLON"
	,"NICKNAME"
	,"SPACE"
	,"SERVERNAME"
	,"EXCLAMATION"
	,"USER"
	,"AT"
	,"HOST"
	,"CMD_PING"
	,"CMD_PONG"
	,"CMD_QUIT"
	,"CMD_PRIVMSG"
	,"PARAM"
	,"TRAILING"
};

const int num_strings = sizeof(test_strings)/sizeof(char*);


void test1()
{
#ifdef TEST_1
	yyscan_t scanner;

	printf("Test 1 starting...\n");

	if (yylex_init(&scanner)) {
		printf("Failed to initialize scanner!");
		return;
	}

	int i;
	for (i = 0; i < num_strings; ++i) {
		YY_BUFFER_STATE state = yy_scan_string(test_strings[i], scanner);

		printf("%s->\n", test_strings[i]);

		int token = 0;
		while (token = yylex(scanner)) {
			printf(" %s %i\n", token2string_table[token], token);
		}

		printf("\n");
		yy_delete_buffer(state, scanner);
	}
	yylex_destroy(scanner);
#endif
}

void test2()
{
#ifdef TEST_2
	yyscan_t scanner;
	void *parser = irc_ParseAlloc(malloc);

	printf("Test 2 starting...\n");

	if (!parser) {
		printf("Failed to initialize parser!");
		return;
	}

	if (yylex_init(&scanner)) {
		printf("Failed to initialize scanner!");
		return;
	}

	irc_ParseTrace(stdout, "IRC ");

	int i;
	for (i = 0; i < num_strings; ++i) {
		YY_BUFFER_STATE state = yy_scan_string(test_strings[i], scanner);

		printf("%s\n", test_strings[i]);

		int token = 0;
		char *data = NULL;
		struct cq_irc_message message = { 0 };


		while (token = yylex(scanner, &data)) {

			printf("Token %i: %s\n", token, data);
			irc_Parse(parser, token, data, &message);

			data = NULL;
		}

		irc_Parse(parser, 0, NULL, &message);

		printf("Host: %s\n", message.prefix.host);
		printf("Source: %s\n", message.prefix.source);
		printf("User: %s\n", message.prefix.user);

		int j;
		for (j = 0; j < message.params.length; ++j) {
			printf("Parameter #%i: %s\n", j, message.params.param[j]);
		}

		printf("Trailing: %s\n", message.trailing);

		yy_delete_buffer(state, scanner);
	}
	yylex_destroy(scanner);
	irc_ParseFree(parser, free);
#endif
}

int main()
{
	test1();
	test2();
}