#include "irc-client-internal.h++"
#include "irc-lex.h++"
#include "format.h"

namespace {

	using namespace std::placeholders;



	void thread_parse(cq_irc_session *session, mutable_buffer buf)
	{

		/* Flexical analyzer */
		yyscan_t scanner;
		YY_BUFFER_STATE state;

		if (yylex_init_extra(session, &scanner) != 0) {
			printf("Failed to initialize Flexical Analyzer.\n");
			return;
		}

		state = yy_scan_buffer(buffer_cast<char*>(buf), buffer_size(buf), scanner);

		if (!state) {
			printf("Failed to initialize Flexical state.\n");
			goto fail0;
		}

		{
			int result = yylex(scanner);

			if (result > 0) { /* Skipped */
				printf("Missing callback, skipped parsing stage.\n");
			}
			if (result < 0) { /* Error */
				printf("Failed to parse message!\n");
			}
		}

		delete[] buffer_cast<char*>(buf);
		yy_delete_buffer(state, scanner);
	
	fail0:
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

		if (error == error::eof ) {
			session->callbacks.signal_disconnect(session);
			return;
		} else if (error == error::operation_aborted) {
			if (!session->socket.is_open()) {
				session->callbacks.signal_disconnect(session);
				printf("Connection closed by client.\n");
				return;
			}
		} else if (error) {
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

		session->callbacks.signal_connect(session);

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

	void on_write(const error_code& error, std::size_t bytes_written, char* buffer)
	{
		if (error) {
			printf("Write error: %s\n", error.message().c_str());
		}

		delete [] buffer;
	}
}

extern "C" {

cq_irc_session *cq_irc_session_connect(
	struct cq_irc_service* service,
	const char* host, 
	const char *port,
	struct cq_irc_callbacks* callbacks)
{
	cq_irc_session *session = new cq_irc_session(service);
	ip::tcp::resolver::query query(host, port);
	auto handler = std::bind(on_resolve, _1, _2, session);

	session->resolver.async_resolve(
		query, handler);

	session->callbacks = *callbacks;

	return session;
}

void cq_irc_session_disconnect(struct cq_irc_session *session)
{
	session->socket.shutdown(ip::tcp::socket::shutdown_both);
	session->socket.close();
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

/* Fetches associated service from the session */
struct cq_irc_service *cq_irc_session_get_service(struct cq_irc_session* session)
{
	return session->service;
}

void cq_irc_session_write(struct cq_irc_session *session, const char* msg, const int size)
{
	if (!msg ||	!size) return;

	char *_buffer = new char[size + 2];
	memcpy(_buffer, msg, size);
	memcpy(_buffer + size, "\r\n", 2);

	async_write(
		session->socket,
		buffer(_buffer, size + 2),
		session->output_strand.wrap(std::bind(on_write, std::placeholders::_1, std::placeholders::_2, _buffer)));
}

void cq_irc_session_write_sync(struct cq_irc_session *session, const char* msg, const int size)
{
	if (!msg ||	!size) return;

	char *_buffer = new char[size + 2];
	memcpy(_buffer, msg, size);
	memcpy(_buffer + size, "\r\n", 2);

	write(session->socket, buffer(_buffer, size+2));

	delete _buffer;
}

void cq_irc_session_pong(struct cq_irc_session* session, const char *ping)
{
	fmt::Writer out;
	out.Format("PONG {0}", ping);

	cq_irc_session_write(session, out.data(), out.size());
}

void cq_irc_session_privmsg(struct cq_irc_session* session, const char* channel, const char* message)
{
	fmt::Writer out;

	out.Format("PRIVMSG {0} :{1}", channel, message);

	cq_irc_session_write(session, out.data(), out.size());
}

void cq_irc_session_quit(struct cq_irc_session* session, const char *message)
{
	fmt::Writer out;

	if (message)
		out.Format("QUIT :{0}", message);
	else
		out.Format("QUIT");

	cq_irc_session_write(session, out.data(), out.size());
}

}