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
	irc_signal_t signal_connect;
	irc_signal_t signal_quit;
	irc_signal_t signal_ping;
	irc_signal_t signal_pong;
	irc_signal_t signal_privmsg;
	irc_signal_t signal_notice;
	irc_signal_t signal_error;
};

/* Service Functions */

/* By "service", I mean underlying networking layer and resources. 
	These functions control this layer. This layer should be interchangeable with other backends later.
	For now, it's reliant upon Boost ASIO. */

/* service_attach() is a blocking function that does nothing but run handlers/events. It will not return until you call stop() */
void cq_irc_service_attach(struct cq_irc_service*);

/* service_poll() is a non-blocking function that will run current available handlers/events. */
void cq_irc_service_poll(struct cq_irc_service*);

/* This tells the underlying network layer to stop processing altogether... you need to destroy sessions first. */
void cq_irc_service_stop(struct cq_irc_service*);

/* Basic service create/destroy functions. 
	TODO: Memory management functionality later on.*/
struct cq_irc_service *cq_irc_service_create();
void cq_irc_service_destroy(struct cq_irc_service*);

/* These functions control plugins associate with a service.
	Any session associated with the service will have the given plugin callbacks run. */
struct cq_irc_plugin *cq_irc_service_plugin_add(struct cq_irc_service*, struct cq_irc_callbacks*);
void cq_irc_service_plugin_remove(struct cq_irc_service*, struct cq_irc_plugin*);

/* Basic session creation/destroy functions. create function also connects.
	TODO: Consider adding more types of creation functions, specifically one that doesn't connect immediately. */
struct cq_irc_session *cq_irc_session_create(struct cq_irc_service*, const char* host, const char *port);
void cq_irc_session_destroy(struct cq_irc_session *session);

/* Thes control plugins and associate them with a session. Only the specified session will run the provided callbacks. */
struct cq_irc_plugin *cq_irc_session_add_plugin(struct cq_irc_session *session, struct cq_irc_callbacks*);
void cq_irc_session_remove_plugin(struct cq_irc_session *session, struct cq_irc_plugin*);

/* This function is how you write into the session. You write size bytes of message to session. 
	NOTE: This function is asynchronous. It returns immediately. 
	NOTE: The provided string will be deep copied! You do not need to keep ownership. 
	TODO: Perhaps add a syncronous version so buffer ownership is possible. 
	TODO: Perhaps more variations on how to pass a buffer.  */
void cq_irc_session_write(struct cq_irc_session *session, const char* message, const int size);

/* This isn't implemented yet... but it will load a shared library 
	then load all of the associated callbacks, if they exist.

	This function will be platform agnostic. No extension should be provided.  */
struct cq_irc_callbacks *cq_irc_callbacks_from_library(const char* library_name);

/* These are proxy functions.
	You can use these if you want. Just fill in a message and session and send a function.
	It will cause the corresponding callback to be called among all associated with the
	session including the services associated. Useful perhaps for testing or user input. */
struct cq_irc_proxy {
	struct cq_irc_message *message;
	struct cq_irc_session *session;
};

void signal_connect_proxy(struct cq_irc_proxy* proxy);
void signal_error_proxy(struct cq_irc_proxy* proxy);
void signal_privmsg_proxy(struct cq_irc_proxy* proxy);
void signal_ping_proxy(struct cq_irc_proxy* proxy);
void signal_pong_proxy(struct cq_irc_proxy* proxy);
void signal_notice_proxy(struct cq_irc_proxy* proxy);
void signal_quit_proxy(struct cq_irc_proxy* proxy);

#ifdef __cplusplus
}
#endif