#ifndef __MESSAGE_PROCESSOR_H__
#define __MESSAGE_PROCESSOR_H__

/**
 * Copyright 2014 Da Zheng
 *
 * This file is part of SA-GraphLib.
 *
 * SA-GraphLib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SA-GraphLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SA-GraphLib.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <memory>

#include "container.h"

#include "messaging.h"
#include "vertex.h"

class graph_engine;
class worker_thread;
class steal_state_t;
class compute_vertex;

/**
 * This class is to process the messages sent to the owner thread.
 * The complexity here is that some messages can't be processed when
 * we try to process them. Therefore, we need an extra buffer to keep them
 * and process them later.
 */
class message_processor
{
	graph_engine &graph;
	worker_thread &owner;

	std::shared_ptr<slab_allocator> msg_alloc;

	// The queue of messages sent from other threads.
	msg_queue msg_q;

	// The thread state of handling stealing vertices.
	std::unique_ptr<steal_state_t> steal_state;

	// This is a message buffer to keep all messages whose destination vertices
	// have been stolen by other threads.
	fifo_queue<message> stolenv_msgs;

	void buf_msg(vertex_message &msg);
	void buf_mmsg(local_vid_t id, multicast_message &mmsg);

	void process_msg(message &msg, bool check_steal);
	void process_multicast_msg(multicast_message &mmsg, bool check_steal);

public:
	message_processor(graph_engine &_graph, worker_thread &_owner,
			std::shared_ptr<slab_allocator> msg_alloc);

	void process_msgs();

	void steal_vertices(compute_vertex *vertices[], int num);
	void return_vertices(vertex_id_t ids[], int num);

	msg_queue &get_msg_queue() {
		return msg_q;
	}

	void reset();
};

#endif
