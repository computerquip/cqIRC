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
	void(*signal_disconnect)(struct cq_irc_session*);
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

/* Fetches associated service from the session */
struct cq_irc_service *cq_irc_session_get_service(struct cq_irc_session*);

/* Basic session creation/destroy functions. create function also connects.
	TODO: Consider adding more types of creation functions, specifically one that doesn't connect immediately. */
struct cq_irc_session *cq_irc_session_create(struct cq_irc_service*, const char* host, const char *port, struct cq_irc_callbacks *);
void cq_irc_session_destroy(struct cq_irc_session *session);

/* This function is how you write into the session. You write size bytes of message to session. 
	NOTE: This function is asynchronous. It returns immediately. 
	NOTE: The provided string will be deep copied! You do not need to keep ownership. 
	TODO: Perhaps add a syncronous version so buffer ownership is possible. 
	TODO: Perhaps more variations on how to pass a buffer.  */
void cq_irc_session_write(struct cq_irc_session *session, const char* message, const int size);
void cq_irc_session_write_sync(struct cq_irc_session *session, const char* msg, const int size);

/* This isn't implemented yet... but it will load a shared library 
	then load all of the associated callbacks, if they exist.

	This function will be platform agnostic. No extension should be provided.  */
struct cq_irc_callbacks *cq_irc_callbacks_from_library(const char* library_name);

/* These functions are counterparts to the above for plugins.
   Plugins have to be dealt with specially else we may cause
   data races. For instance, it's okay to destroy the session
   in a plugin, 
   but only after all current frame callbacks have finished, else
   they would recieve a bad session object (access free'd memory). 
 */ 

void cq_irc_session_privmsg(struct cq_irc_session* session, const char* channel, const char* message);
void cq_irc_session_pong(struct cq_irc_session*, const char* ping);
void cq_irc_session_quit(struct cq_irc_session* session, const char *message);

#ifdef __cplusplus
}
#endif