#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cq_irc_service;
struct cq_irc_session;
struct cq_irc_plugin;

struct cq_irc_prefix {
	char *host;
	char *user;
	char *source;
};

struct cq_irc_params {
	char *param[14];
	uint8_t length;
};

struct cq_irc_message {
	struct cq_irc_prefix prefix;
	struct cq_irc_params params;
	char *trailing;
};

typedef void (*irc_signal_t)(struct cq_irc_message* data);

struct cq_irc_callbacks {
	irc_signal_t signal_connect;
	irc_signal_t signal_quit;
	irc_signal_t signal_ping;
	irc_signal_t signal_pong;
	irc_signal_t signal_privmsg;
	irc_signal_t signal_notice;
	irc_signal_t signal_error;
};

void cq_irc_attach();
void cq_irc_poll();
void cq_irc_stop();

struct cq_irc_service *cq_irc_service_create();
void cq_irc_service_attach(struct cq_irc_service*);
void cq_irc_service_poll(struct cq_irc_service*);
void cq_irc_service_stop(struct cq_irc_service*);
void cq_irc_service_destroy(struct cq_irc_service*);

struct cq_irc_service *cq_irc_default_service();

struct cq_irc_plugin *cq_irc_service_plugin_add(struct cq_irc_service*, struct cq_irc_callbacks*);
void cq_irc_service_plugin_remove(struct cq_irc_service*, struct cq_irc_plugin*);

struct cq_irc_plugin *cq_irc_plugin_add(struct cq_irc_callbacks*);
void cq_irc_plugin_remove(struct cq_irc_plugin*);

struct cq_irc_session *cq_irc_session_connect(const char* host, const char *port);
void cq_irc_session_free(struct cq_irc_session *session);

struct cq_irc_plugin *cq_irc_session_add_plugin(struct cq_irc_session *session, struct cq_irc_callbacks*);
void cq_irc_session_remove_plugin(struct cq_irc_session *session, struct cq_irc_plugin*);

struct cq_irc_callbacks *cq_irc_callbacks_from_library(const char* library_name);

#ifdef __cplusplus
}
#endif