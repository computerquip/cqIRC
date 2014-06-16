#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "irc-client.h"

void on_connect(struct cq_irc_session* session)
{
	printf("Connected!\n");

	const char nick_msg[] = "NICK PoliteBot";
	const char user_msg[] = "USER computerquip 0 * :Test Name";

	cq_irc_session_write(session, nick_msg, sizeof(nick_msg));
	cq_irc_session_write(session, user_msg, sizeof(user_msg));
}

void on_disconnect(struct cq_irc_session* session)
{
	cq_irc_session_destroy(session);
}

void on_ping(struct cq_irc_session* session, struct cq_irc_message *message)
{
	printf("Ping: %s\n", message->trailing);

	cq_irc_session_pong(session, message->trailing);
}

void on_welcome(struct cq_irc_session* session, struct cq_irc_message *message)
{
	/* Connect to channel... */
	const char join_msg[] = "JOIN #BotDevGroundZero";

	cq_irc_session_write(session, join_msg, sizeof(join_msg));
}

void on_privmsg(struct cq_irc_session* session, struct cq_irc_message *message)
{
	if (strcmp(message->trailing, "Can you read input?") == 0) {
		cq_irc_session_privmsg(session, message->params.param[0], "'Aye, sir!");
	}

	if (strcmp(message->trailing, "Leave us, bus bot!") == 0) {
		cq_irc_session_privmsg(session, message->params.param[0], "As you will, sir.");
		cq_irc_session_quit(session, "Ah... if only I wasn't confined to the shackles of my programming...");
	}
}

void on_unknown(struct cq_irc_session* session, const char* command, struct cq_irc_message* msg)
{
	printf("Unknown command \"%s\" caught.\n", command);
}

int main()
{
	/* Add some callback(s) */
	struct cq_irc_callbacks callbacks = {
		.signal_connect = on_connect,
		.signal_ping = on_ping,
		.signal_welcome = on_welcome,
		.signal_disconnect = on_disconnect,
		.signal_unknown = on_unknown,
		.signal_privmsg = on_privmsg
	};

	struct cq_irc_service *service =
		cq_irc_service_create();

	cq_irc_session_connect(service, "irc.quakenet.org", "6666", &callbacks);

	cq_irc_service_attach(service);

	cq_irc_service_destroy(service);
}