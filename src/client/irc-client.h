#pragma once

struct cq_irc_service;
struct cq_irc_session;

struct cq_irc_callbacks {
	void (*signal_connect)(void);
};