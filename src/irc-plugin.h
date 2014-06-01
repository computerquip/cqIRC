#pragma once

#include "irc-client.h"

struct cq_irc_plugin {
	struct cq_irc_callbacks callbacks;
	struct cq_irc_plugin *next;
};