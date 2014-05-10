#include <boost/asio.hpp>
#include <functional>
#include <cstdio>

#include "irc.lex.h"

#include "irc-types.h"
#include "irc-client.h"

using namespace boost::system;
using namespace boost::asio;

struct cq_irc_session {
	cq_irc_session(io_service &service)
		: socket(service), resolver(service), work(service) { }

	ip::tcp::socket socket;
	ip::tcp::resolver resolver;
	io_service::work work;
	streambuf input, output;

	cq_irc_callbacks *callbacks;
};

namespace {

using namespace std::placeholders;

/* We could just pass a context around instead.
   Do we gain anything from letting the user spawn more io_service instances?
   I don't beliesve we do... so we're going global.  */
io_service g_service;

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

void threaded_parse(cq_irc_session *session, mutable_buffer buf)
{

	/* Flexical analyzer */
	yyscan_t scanner;
	YY_BUFFER_STATE state;
    struct cq_irc_message message = { 0 };
    char *data = NULL;

	if (yylex_init_extra(&message, &scanner) != 0) {
		printf("Failed to initialize Flexical Analyzer.\n");
		return;
	}

	state = yy_scan_buffer(buffer_cast<char*>(buf), buffer_size(buf), scanner);

	if (!state) {
		printf("Failed to initialize Flexical state.\n");
		return;
	}

	printf("Message: %*s", buffer_size(buf), buffer_cast<const char*>(buf));

	{
		int result = yylex(scanner);

		if (result != 0) { /* Error */
			printf("Failed to parse message!\n");
			return;
		}
	}

	printf("Host: %s\n", message.prefix.host);
	printf("Source: %s\n", message.prefix.source);
	printf("User: %s\n", message.prefix.user);

	for (int j = 0; j < message.params.length; ++j) {
		printf("Parameter #%i: %s\n", j, message.params.param[j]);
	}

	printf("Trailing: %s\n", message.trailing);

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

	auto parse_work = std::bind(threaded_parse, session, buf);

	g_service.post(parse_work);
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

	session->callbacks->signal_connect();

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
	g_service.run();
}

void cq_irc_poll()
{
	g_service.poll();
}

void cq_irc_stop()
{
	g_service.stop();
}

cq_irc_session *cq_irc_session_connect(
	const char* host, const char *port, 
	struct cq_irc_callbacks *callbacks)
{
	cq_irc_session *session = new cq_irc_session(g_service);
	ip::tcp::resolver::query query(host, port);
	auto handler = std::bind(on_resolve, _1, _2, session);

	session->callbacks = callbacks;

	session->resolver.async_resolve(
		query, handler);

	return session;
}

void cq_irc_session_free(struct cq_irc_session *session)
{
	delete session;
}

void on_connect()
{
	printf("We are technically connected to the server!\n");
}

int main()
{
	struct cq_irc_callbacks callbacks = {
		.signal_connect = on_connect
	};

	struct cq_irc_session *session = cq_irc_session_connect("irc.quakenet.org", "6667", &callbacks);
	cq_irc_attach();
	cq_irc_session_free(session);
}

}