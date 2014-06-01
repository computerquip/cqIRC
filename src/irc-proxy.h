#pragma once
#include "irc-plugin.h"

/*	Here, we define our plugin types. 
	We also proxy plugin calls to all plugins. */

struct cq_irc_proxy;

typedef void (*irc_proxy_signal_t)(struct cq_irc_proxy *proxy);

struct cq_irc_proxy {
	irc_proxy_signal_t signal; /* Used with Flex... perhaps should be removed. */
	struct cq_irc_message *message;
	struct cq_irc_plugin *service_plugins;
	struct cq_irc_plugin *session_plugins;
};

#define IRC_PROXY_FUNC(name) \
	void signal_##name##_proxy(struct cq_irc_proxy* proxy);

IRC_PROXY_FUNC(connect)
IRC_PROXY_FUNC(error)
IRC_PROXY_FUNC(privmsg)
IRC_PROXY_FUNC(ping)
IRC_PROXY_FUNC(pong)
IRC_PROXY_FUNC(notice)
IRC_PROXY_FUNC(quit)

#undef IRC_PROXY_FUNC