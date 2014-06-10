#include "irc-client-internal.h++"
#include "irc-lex.h++"

namespace {

	using namespace std::placeholders;

	void cq_irc_message_destroy(cq_irc_message* msg)
	{
		free(msg->prefix.host);
		free(msg->prefix.source);
		free(msg->prefix.user);

		for (int j = 0; j < msg->params.length; ++j) {
			free(msg->params.param[j]);
		}

		free(msg->trailing);
	}

	void thread_parse(cq_irc_session *session, mutable_buffer buf)
	{

		/* Flexical analyzer */
		yyscan_t scanner;
		YY_BUFFER_STATE state;
	    struct cq_irc_message message = { 0 };

	    struct cq_irc_proxy proxy = {
			.message = &message,
			.session = session,
		};

		struct cq_irc_flex_info info = {
			.signal = NULL,
			.proxy = &proxy
		};


		if (yylex_init_extra(&info, &scanner) != 0) {
			printf("Failed to initialize Flexical Analyzer.\n");
			return;
		}

		state = yy_scan_buffer(buffer_cast<char*>(buf), buffer_size(buf), scanner);

		if (!state) {
			printf("Failed to initialize Flexical state.\n");
			return;
		}

		{
			int result = yylex(scanner);

			if (result != 0) { /* Error */
				printf("Failed to parse message!\n");
				return;
			}
		}

		cq_irc_message_destroy(&message);

		delete[] buffer_cast<char*>(buf);
		yy_delete_buffer(state, scanner);
		yylex_destroy(scanner);
	}

	void on_read(
	  const error_code& error,
	  std::size_t msg_size,
	  cq_irc_session *session)
	{
		auto handler = std::bind(on_read, _1, _2, session);

		/* Extra two bytes are for Flex so we don't require a copy buffer. */
		int buf_size = msg_size + 2;

		if (error) {
			printf("Reading Error: %s\n", error.message().c_str());
			return;
		}

		mutable_buffer buf(new char[buf_size](), buf_size);

		buffer_copy(buf, session->input.data(), msg_size);
		session->input.consume(msg_size);

		async_read_until(
			session->socket, session->input,
			"\r\n",	handler);

		auto parse_work = std::bind(thread_parse, session, buf);

		session->service->service.post(parse_work);
	}

	void on_connect(
		const error_code& error,
		ip::tcp::resolver::iterator iterator,
		cq_irc_session *session)
	{
		auto handler = std::bind(on_read, _1, _2, session);

		if (error) {
			printf("Connection Error: %s\n", error.message().c_str());
			return;
		}

		{
			struct cq_irc_proxy proxy = 
				{ NULL, session };

			signal_connect_proxy(&proxy);
		}

		async_read_until(
			session->socket, session->input,
			"\r\n",	handler);
	}

	void on_resolve(
		const error_code& error, 
		ip::tcp::resolver::iterator iterator,
		cq_irc_session *session)
	{
		auto handler = std::bind(on_connect, _1, _2, session);

		if (error) {
			printf("Resolver error: %s\n", error.message().c_str());
			return;
		}

		async_connect(
			session->socket, iterator, handler);
	}

	void on_write(const error_code& error, std::size_t bytes_written)
	{
		if (error) {
			printf("Write error: %s\n", error.message().c_str());
			return;
		}
	}
}

extern "C" {

cq_irc_session *cq_irc_session_create(
	struct cq_irc_service* service,
	const char* host, 
	const char *port)
{
	cq_irc_session *session = new cq_irc_session(service);
	ip::tcp::resolver::query query(host, port);
	auto handler = std::bind(on_resolve, _1, _2, session);

	session->resolver.async_resolve(
		query, handler);

	return session;
}

void cq_irc_session_destroy(struct cq_irc_session *session)
{
	delete session;
}

struct cq_irc_service *cq_irc_service_create()
{
	return new cq_irc_service;
}

void cq_irc_service_destroy(struct cq_irc_service* service)
{
	if (!service) return;

	while (!service->plugins) {
		struct cq_irc_plugin *tmp = service->plugins;
		service->plugins = service->plugins->next;
		
		delete tmp;
	}

	service->service.stop();

	delete service;
}

void cq_irc_service_attach(struct cq_irc_service* service)
{
	service->service.run();
}

void cq_irc_service_poll(struct cq_irc_service* service)
{
	service->service.poll();
}

void cq_irc_service_stop(struct cq_irc_service* service)
{
	service->service.stop();
}

/* Plugin stuff! */
namespace {

	struct cq_irc_plugin *add_plugin(struct cq_irc_plugin* plugin, struct cq_irc_plugin** plugins)
	{
		if (!plugin) return NULL;

		if (*plugins) {
			plugin->next = *plugins;
			*plugins = plugin;
		} else {
			*plugins = plugin;
		}

		return plugin;
	}

	void remove_plugin(struct cq_irc_plugin* plugin, struct cq_irc_plugin** plugins)
	{
		

		if (plugin == *plugins) {
			delete plugin;
			*plugins = (*plugins)->next;
			return;
		}

		{
			struct cq_irc_plugin *iterator = (*plugins)->next;
			struct cq_irc_plugin *prev = *plugins;

			for (; iterator != NULL; prev = iterator, iterator = iterator->next)
			{
				if (plugin == iterator) {
					prev->next = iterator->next;

					delete iterator;

					return;
				}
			}
		}
	}

}


struct cq_irc_plugin *cq_irc_service_plugin_add(
	struct cq_irc_service* service, 
	struct cq_irc_callbacks* callbacks)
{
	if (!callbacks) return NULL;
	if (!service) return NULL;

	struct cq_irc_plugin *plugin = new cq_irc_plugin;

	if (!plugin) return NULL;

	plugin->callbacks = *callbacks;

	service->mutex.lock();
		add_plugin(plugin, &service->plugins);
	service->mutex.unlock();

	return plugin;
}

void cq_irc_service_plugin_remove(
	struct cq_irc_service* service, 
	struct cq_irc_plugin* plugin)
{

	if (!service) return;

	service->mutex.lock();
		remove_plugin(plugin, &service->plugins);
	service->mutex.unlock();

}

struct cq_irc_plugin *cq_irc_session_add_plugin(
	struct cq_irc_session *session, 
	struct cq_irc_callbacks* callbacks)
{
	if (!callbacks) return NULL;
	if (!session) return NULL;

	struct cq_irc_plugin *plugin = new cq_irc_plugin;

	if (!plugin) return NULL;

	plugin->callbacks = *callbacks;

	session->mutex.lock();
		add_plugin(plugin, &session->plugins);
	session->mutex.unlock();

	return plugin;
}

void cq_irc_session_remove_plugin(
	struct cq_irc_session *session, 
	struct cq_irc_plugin* plugin)
{
	if (!session) return;

	session->mutex.lock();
		remove_plugin(plugin, &session->plugins);
	session->mutex.unlock();
}

void cq_irc_session_write(struct cq_irc_session *session, const char* msg, const int size)
{
	if (!msg ||	!size) return;

	char *_buffer = new char[size + 2];
	memcpy(_buffer, msg, size);
	memcpy(_buffer + size, "\r\n", 2);

	async_write(
		session->socket,
		buffer(_buffer, size),
		session->output_strand.wrap(on_write));
}

/* Proxy Function Definitions */

#define IRC_PLUGIN_FOR(proxy, name, plugins) \
	for (struct cq_irc_plugin *plugin = plugins; \
		plugin != NULL; \
		plugin = plugin->next) \
	{ \
		if (plugin->callbacks.signal_##name) \
			plugin->callbacks.signal_##name(proxy->session, proxy->message); \
	} \

#define IRC_PROXY_FUNC(name) \
	void signal_##name##_proxy(struct cq_irc_proxy* proxy) \
	{ \
		if (proxy->session->service->plugins) \
			IRC_PLUGIN_FOR(proxy, name, proxy->session->service->plugins) \
		\
		if (proxy->session->plugins) \
			IRC_PLUGIN_FOR(proxy, name, proxy->session->plugins) \
	} \

IRC_PROXY_FUNC(connect)
IRC_PROXY_FUNC(error)
IRC_PROXY_FUNC(privmsg)
IRC_PROXY_FUNC(ping)
IRC_PROXY_FUNC(pong)
IRC_PROXY_FUNC(notice)
IRC_PROXY_FUNC(quit)

}