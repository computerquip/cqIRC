#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irc-client.h"

void on_connect(struct cq_irc_session* session, struct cq_irc_message *message)
{
	printf("Connected!\n");

	const char nick_msg[] = "NICK CQBot";
	const char user_msg[] = "USER computerquip 0 * :Test Name";

	cq_irc_session_write(session, nick_msg, sizeof(nick_msg) - 1);
	cq_irc_session_write(session, user_msg, sizeof(user_msg) - 1);
}

void on_notice(struct cq_irc_session* session, struct cq_irc_message *message)
{
	printf("Notice Message: ");

	for (int i = 0; i < message->params.length; ++i) {
		printf("%s ", message->params.param[i]);
	}

	printf("%s", message->trailing);

	printf("\n");
}

void on_ping(struct cq_irc_session* session, struct cq_irc_message *message)
{
	printf("Ping: %s\n", message->trailing);

	int size = sizeof("PONG :") + strlen(message->trailing) - 1;
	char *msg = malloc(size);
	sprintf(msg, "PONG :%s", message->trailing);

	cq_irc_session_write(session, msg, size);

	/* Connect to channel... */
	const char join_msg[] = "JOIN #cplusplus";

	cq_irc_session_write(session, join_msg, sizeof(join_msg));

	free(msg);
}

int main()
{
	/* Add some callback(s) */
	struct cq_irc_callbacks callbacks = {
		.signal_connect = on_connect,
		.signal_notice = on_notice,
		.signal_ping = on_ping
	};

	struct cq_irc_service *service =
		cq_irc_service_create();

	cq_irc_service_plugin_add(service, &callbacks);

	struct cq_irc_session *session = 
		cq_irc_session_create(service, "irc.quakenet.org", "6666");

	cq_irc_service_attach(service);

	cq_irc_session_destroy(session);
	cq_irc_service_destroy(service);
}