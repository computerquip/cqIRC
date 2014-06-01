#include "irc-proxy.h"
#include <stdlib.h>

#define IRC_PLUGIN_FOR(name, plugins) \
	for (struct cq_irc_plugin *plugin = plugins; \
		plugin != NULL; \
		plugin = plugin->next) \
	{ \
		plugin->callbacks.signal_##name (proxy->message); \
	} \

#define IRC_PROXY_FUNC(name) \
	void signal_##name##_proxy(struct cq_irc_proxy* proxy) \
	{ \
		IRC_PLUGIN_FOR(name, proxy->service_plugins) \
		IRC_PLUGIN_FOR(name, proxy->session_plugins) \
	} \

IRC_PROXY_FUNC(connect)
IRC_PROXY_FUNC(error)
IRC_PROXY_FUNC(privmsg)
IRC_PROXY_FUNC(ping)
IRC_PROXY_FUNC(pong)
IRC_PROXY_FUNC(notice)
IRC_PROXY_FUNC(quit)