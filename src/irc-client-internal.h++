#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <mutex>
#include <cstdio>

#include "irc-client.h"

using namespace boost::system;
using namespace boost::asio;

struct cq_irc_service {
	io_service service;
};

struct cq_irc_session {
	cq_irc_session(struct cq_irc_service *_service)
		: socket(_service->service), resolver(_service->service), 
		  work(_service->service), output_strand(_service->service),
		  service(_service)
	{ }

	ip::tcp::socket socket;
	ip::tcp::resolver resolver;
	io_service::work work;
	streambuf input;
	strand output_strand; /* Prevents concurrent write requests. */

	struct cq_irc_callbacks callbacks;
	struct cq_irc_service *service;
	int use_generic = 0;
};