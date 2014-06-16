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

typedef void (*irc_signal_t)(struct cq_irc_session*, struct cq_irc_message*);

struct cq_irc_callbacks {
	void(*signal_connect)(struct cq_irc_session*);
	irc_signal_t signal_welcome;
	irc_signal_t signal_ping;
	irc_signal_t signal_privmsg;
	irc_signal_t signal_notice;
	irc_signal_t signal_error;
	void(*signal_unknown)(struct cq_irc_session*, const char* command, struct cq_irc_message*);
	void(*signal_disconnect)(struct cq_irc_session*);
};


void cq_irc_service_attach(struct cq_irc_service*);
void cq_irc_service_poll(struct cq_irc_service*);
void cq_irc_service_stop(struct cq_irc_service*);
struct cq_irc_service *cq_irc_service_create();
void cq_irc_service_destroy(struct cq_irc_service*);

struct cq_irc_service *cq_irc_session_get_service(struct cq_irc_session*);
struct cq_irc_session *cq_irc_session_connect(struct cq_irc_service*, const char* host, const char *port, struct cq_irc_callbacks *);
void cq_irc_session_disconnect(struct cq_irc_session*);
void cq_irc_session_destroy(struct cq_irc_session *session);
void cq_irc_session_write(struct cq_irc_session *session, const char* message, const int size);
void cq_irc_session_write_sync(struct cq_irc_session *session, const char* msg, const int size);
struct cq_irc_callbacks *cq_irc_callbacks_from_library(const char* library_name);
void cq_irc_session_privmsg(struct cq_irc_session* session, const char* channel, const char* message);
void cq_irc_session_pong(struct cq_irc_session*, const char* ping);
void cq_irc_session_quit(struct cq_irc_session* session, const char *message);

#ifdef __cplusplus
}
#endif