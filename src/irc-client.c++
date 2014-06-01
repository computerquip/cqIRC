#include <boost/asio.hpp>
#include <functional>
#include <mutex>
#include <cstdio>

/* These must be exposed appropriately else we get C++ mangled signatures.  */
extern "C" {
#include "irc.lex.h"
#include "irc-proxy.h"
}

using namespace boost::system;
using namespace boost::asio;

struct cq_irc_service {
	io_service service;
	struct cq_irc_plugin *plugins = NULL;
	std::mutex mutex;
};

struct cq_irc_session {
	cq_irc_session(struct cq_irc_service *_service)
		: socket(_service->service), resolver(_service->service), 
		  work(_service->service), service(_service) { }

	ip::tcp::socket socket;
	ip::tcp::resolver resolver;
	io_service::work work;
	streambuf input, output;

	struct cq_irc_service *service;
	struct cq_irc_plugin *plugins;
	std::mutex mutex;
};

namespace {

using namespace std::placeholders;

struct cq_irc_service g_service;

void cq_irc_message_free(cq_irc_message* msg)
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
    	.signal = 0,
		.message = &message,
		.service_plugins = session->service->plugins,
		.session_plugins = session->plugins,
	};

	if (yylex_init_extra(&proxy, &scanner) != 0) {
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

	cq_irc_message_free(&message);

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
		printf("Failed to read!\n");
		return;
	}

	mutable_buffer buf(new char[buf_size](), buf_size);

	buffer_copy(buf, session->input.data(), msg_size);
	session->input.consume(msg_size);

	async_read_until(
		session->socket, session->input,
		"\r\n",	handler);

	auto parse_work = std::bind(thread_parse, session, buf);

	g_service.service.post(parse_work);
}

void on_connect(
	const error_code& error,
	ip::tcp::resolver::iterator iterator,
	cq_irc_session *session)
{
	auto handler = std::bind(on_read, _1, _2, session);

	if (error) {
		printf("Failed to connect!\n");
		return;
	}

	{
		struct cq_irc_proxy proxy = {
			0, NULL,
			session->service->plugins,
			session->plugins
		};

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
		printf("Failed to resolve the address!\n");
		return;
	}

	printf("We have resolved... the issue. It was very punny.\n");

	async_connect(
		session->socket, iterator, handler);
}

} //Nameless namespace

extern "C" {

void cq_irc_attach()
{
	g_service.service.run();
}

void cq_irc_poll()
{
	g_service.service.poll();
}

void cq_irc_stop()
{
	g_service.service.stop();
}

cq_irc_session *cq_irc_session_connect(
	const char* host, 
	const char *port)
{
	cq_irc_session *session = new cq_irc_session(&g_service);
	ip::tcp::resolver::query query(host, port);
	auto handler = std::bind(on_resolve, _1, _2, session);

	session->resolver.async_resolve(
		query, handler);

	return session;
}

void cq_irc_session_free(struct cq_irc_session *session)
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

struct cq_irc_service *cq_irc_default_service()
{
	return &g_service;
}

/* Plugin stuff! */
namespace {

struct cq_irc_plugin *add_plugin(struct cq_irc_plugin* plugin, struct cq_irc_plugin** plugins)
{
	if (plugin) return NULL;

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

struct cq_irc_plugin *cq_irc_plugin_add(struct cq_irc_callbacks* callbacks)
{
	return cq_irc_service_plugin_add(&g_service, callbacks);
}

void cq_irc_plugin_remove(struct cq_irc_plugin* plugin)
{
	cq_irc_service_plugin_remove(&g_service, plugin);
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

}