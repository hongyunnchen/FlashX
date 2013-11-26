/**
 * Copyright 2013 Da Zheng
 *
 * This file is part of SAFSlib.
 *
 * SAFSlib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SAFSlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SAFSlib.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include <tr1/unordered_set>
#include <vector>

#include "RAID_config.h"
#include "io_interface.h"
#include "read_private.h"
#include "direct_private.h"
#include "aio_private.h"
#include "remote_access.h"
#include "global_cached_private.h"
#include "part_global_cached_private.h"
#include "cache_config.h"
#include "disk_read_thread.h"
#include "debugger.h"
#include "mem_tracker.h"
#include "native_file.h"
#include "safs_file.h"

#define DEBUG

/**
 * This global data collection is very static.
 * Once the data is initialized, no data needs to be changed.
 * The mutex is to used only at the initialization.
 * As long as all threads call init_io_system() first before using
 * the global data, they will all see the complete global data.
 */
struct global_data_collection
{
	RAID_config raid_conf;
	std::vector<disk_io_thread *> read_threads;
	pthread_mutex_t mutex;

#ifdef DEBUG
	std::tr1::unordered_set<io_interface *> ios;
	pthread_spinlock_t ios_lock;
#endif

	global_data_collection() {
		pthread_mutex_init(&mutex, NULL);
#ifdef DEBUG
		pthread_spin_init(&ios_lock, PTHREAD_PROCESS_PRIVATE);
#endif
	}

#ifdef DEBUG
	void register_io(io_interface *io) {
		pthread_spin_lock(&ios_lock);
		ios.insert(io);
		pthread_spin_unlock(&ios_lock);
	}
#endif
};

static global_data_collection global_data;

class debug_global_data: public debug_task
{
public:
	void run();
};

void debug_global_data::run()
{
	for (unsigned i = 0; i < global_data.read_threads.size(); i++)
		global_data.read_threads[i]->print_state();
#ifdef DEBUG
	pthread_spin_lock(&global_data.ios_lock);
	std::tr1::unordered_set<io_interface *> ios = global_data.ios;
	pthread_spin_unlock(&global_data.ios_lock);

	for (std::tr1::unordered_set<io_interface *>::iterator it
			= ios.begin(); it != ios.end(); it++)
		(*it)->print_state();
#endif
}

const RAID_config &get_sys_RAID_conf()
{
	return global_data.raid_conf;
}

void init_io_system(const config_map &configs)
{
#ifdef ENABLE_MEM_TRACE
	init_mem_tracker();
#endif

	params.init(configs.get_options());
	params.print();

	numa_set_bind_policy(1);
	thread::thread_class_init();

	std::string root_conf_file = configs.get_option("root_conf");
	printf("The root conf file: %s\n", root_conf_file.c_str());
	RAID_config raid_conf(root_conf_file, params.get_RAID_mapping_option(),
			params.get_RAID_block_size());
	int num_files = raid_conf.get_num_disks();
	global_data.raid_conf = raid_conf;

	std::set<int> disk_node_set = raid_conf.get_node_ids();
	std::vector<int> disk_node_ids(disk_node_set.begin(), disk_node_set.end());
	printf("There are %ld nodes with disks\n", disk_node_ids.size());
	init_aio(disk_node_ids);

	file_mapper *mapper = raid_conf.create_file_mapper();
	/* 
	 * The mutex is enough to guarantee that all threads will see initialized
	 * global data. The first thread that enters the critical area will
	 * initialize the global data. If another thread tries to run the code,
	 * it will be blocked by the mutex. When a thread is returned from
	 * the function, they all can see the global data.
	 */
	pthread_mutex_lock(&global_data.mutex);
	// The global data hasn't been initialized.
	if (global_data.read_threads.size() == 0) {
		global_data.read_threads.resize(num_files);
		for (int k = 0; k < num_files; k++) {
			std::vector<int> indices(1, k);
			logical_file_partition partition(indices, mapper);
			// Create disk accessing threads.
			global_data.read_threads[k] = new disk_io_thread(partition,
					global_data.raid_conf.get_disk(k).node_id, NULL, k);
		}
		debug.register_task(new debug_global_data());
	}
	pthread_mutex_unlock(&global_data.mutex);
}

void destroy_io_system()
{
	// TODO
}

class posix_io_factory: public file_io_factory
{
	int access_option;
public:
	posix_io_factory(const std::string &file_name,
			int access_option): file_io_factory(file_name) {
		this->access_option = access_option;
	}

	virtual io_interface *create_io(thread *t);

	virtual void destroy_io(io_interface *io) {
	}
};

class aio_factory: public file_io_factory
{
public:
	aio_factory(const std::string &file_name): file_io_factory(file_name) {
	}

	virtual io_interface *create_io(thread *t);

	virtual void destroy_io(io_interface *io) {
	}
};

class remote_io_factory: public file_io_factory
{
protected:
	file_mapper *mapper;
public:
	remote_io_factory(const std::string &file_name);

	~remote_io_factory();

	virtual io_interface *create_io(thread *t);

	virtual void destroy_io(io_interface *io) {
	}
};

class global_cached_io_factory: public remote_io_factory
{
	const cache_config *cache_conf;
	static page_cache *global_cache;
public:
	global_cached_io_factory(const std::string &file_name,
			const cache_config *cache_conf): remote_io_factory(file_name) {
		this->cache_conf = cache_conf;
		if (global_cache == NULL) {
			global_cache = cache_conf->create_cache(MAX_NUM_FLUSHES_PER_FILE *
					global_data.raid_conf.get_num_disks());
			int num_files = global_data.read_threads.size();
			for (int k = 0; k < num_files; k++) {
				global_data.read_threads[k]->register_cache(global_cache);
			}
		}
	}

	~global_cached_io_factory() {
		cache_conf->destroy_cache(global_cache);
	}

	virtual io_interface *create_io(thread *t);

	virtual void destroy_io(io_interface *io) {
	}
};

class part_global_cached_io_factory: public remote_io_factory
{
	const cache_config *cache_conf;
	part_io_process_table *table;
	int num_nodes;
public:
	part_global_cached_io_factory(const std::string &file_name,
			const cache_config *cache_conf);

	~part_global_cached_io_factory() {
		part_global_cached_io::close_file(table);
	}

	virtual io_interface *create_io(thread *t);

	virtual void destroy_io(io_interface *io) {
	}
};

io_interface *posix_io_factory::create_io(thread *t)
{
	file_mapper *mapper = global_data.raid_conf.create_file_mapper(get_name());
	assert(mapper);
	int num_files = mapper->get_num_files();
	std::vector<int> indices;
	for (int i = 0; i < num_files; i++)
		indices.push_back(i);
	// The partition contains all files.
	logical_file_partition global_partition(indices, mapper);

	io_interface *io;
	switch (access_option) {
		case READ_ACCESS:
			io = new buffered_io(global_partition, t);
			break;
		case DIRECT_ACCESS:
			io = new direct_io(global_partition, t);
			break;
		default:
			fprintf(stderr, "a wrong posix access option\n");
			assert(0);
	}
#ifdef DEBUG
	global_data.register_io(io);
#endif
	return io;
}

io_interface *aio_factory::create_io(thread *t)
{
	file_mapper *mapper = global_data.raid_conf.create_file_mapper(get_name());
	assert(mapper);
	int num_files = mapper->get_num_files();
	std::vector<int> indices;
	for (int i = 0; i < num_files; i++)
		indices.push_back(i);
	// The partition contains all files.
	logical_file_partition global_partition(indices, mapper);

	io_interface *io;
	io = new async_io(global_partition, params.get_aio_depth_per_file(), t);
#ifdef DEBUG
	global_data.register_io(io);
#endif
	return io;
}

remote_io_factory::remote_io_factory(const std::string &file_name): file_io_factory(
			file_name)
{
	mapper = global_data.raid_conf.create_file_mapper(get_name());
	assert(mapper);
	int num_files = mapper->get_num_files();
	assert((int) global_data.read_threads.size() == num_files);

	for (int i = 0; i < num_files; i++) {
		global_data.read_threads[i]->open_file(mapper);
	}
}

remote_io_factory::~remote_io_factory()
{
	int num_files = mapper->get_num_files();
	for (int i = 0; i < num_files; i++)
		global_data.read_threads[i]->close_file(mapper);
	delete mapper;
}

io_interface *remote_io_factory::create_io(thread *t)
{
	io_interface *io = new remote_io(global_data.read_threads, mapper, t);
#ifdef DEBUG
	global_data.register_io(io);
#endif
	return io;
}

io_interface *global_cached_io_factory::create_io(thread *t)
{
	io_interface *underlying = new remote_io(
			global_data.read_threads, mapper, t);
	global_cached_io *io = new global_cached_io(t, underlying,
			global_cache);
#ifdef DEBUG
	global_data.register_io(io);
#endif
	return io;
}

part_global_cached_io_factory::part_global_cached_io_factory(
		const std::string &file_name,
		const cache_config *cache_conf): remote_io_factory(file_name)
{
	std::vector<int> node_id_array;
	cache_conf->get_node_ids(node_id_array);

	std::map<int, io_interface *> underlyings;
	table = part_global_cached_io::open_file(global_data.read_threads,
			mapper, cache_conf);
	this->cache_conf = cache_conf;
	this->num_nodes = node_id_array.size();

}

io_interface *part_global_cached_io_factory::create_io(thread *t)
{
	part_global_cached_io *io = part_global_cached_io::create(
			new remote_io(global_data.read_threads, mapper, t), table);
#ifdef DEBUG
	global_data.register_io(io);
#endif
	return io;
}

file_io_factory *create_io_factory(const std::string &file_name,
		const int access_option)
{
	for (int i = 0; i < global_data.raid_conf.get_num_disks(); i++) {
		std::string abs_path = global_data.raid_conf.get_disk(i).name
			+ "/" + file_name;
		native_file f(abs_path);
		if (!f.exist()) {
			fprintf(stderr, "the underlying file %s doesn't exist\n",
					abs_path.c_str());
			return NULL;
		}
	}

	std::vector<int> node_id_array;
	for (int i = 0; i < params.get_num_nodes(); i++)
		node_id_array.push_back(i);
	cache_config *cache_conf = new even_cache_config(params.get_cache_size(),
				params.get_cache_type(), node_id_array);

	file_io_factory *factory;
	switch (access_option) {
		case READ_ACCESS:
		case DIRECT_ACCESS:
			factory = new posix_io_factory(file_name, access_option);
			break;
		case AIO_ACCESS:
			factory = new aio_factory(file_name);
			break;
		case REMOTE_ACCESS:
			factory = new remote_io_factory(file_name);
			break;
		case GLOBAL_CACHE_ACCESS:
			assert(cache_conf);
			factory = new global_cached_io_factory(file_name, cache_conf);
			break;
		case PART_GLOBAL_ACCESS:
			assert(cache_conf);
			factory = new part_global_cached_io_factory(file_name, cache_conf);
			break;
		default:
			fprintf(stderr, "a wrong access option\n");
			assert(0);
	}
	return factory;
}

void destroy_io_factory(file_io_factory *factory)
{
	delete factory;
}

void print_io_thread_stat()
{
	for (unsigned i = 0; i < global_data.read_threads.size(); i++) {
		disk_io_thread *t = global_data.read_threads[i];
		if (t)
			t->stop();
	}
	sleep(1);
	for (unsigned i = 0; i < global_data.read_threads.size(); i++) {
		disk_io_thread *t = global_data.read_threads[i];
		if (t)
			t->print_stat();
	}
}

ssize_t file_io_factory::get_file_size() const
{
	safs_file f(global_data.raid_conf, name);
	return f.get_file_size();
}

atomic_integer io_interface::io_counter;
page_cache *global_cached_io_factory::global_cache;
