#include <agc.h>

AGC_MODULE_LOAD_FUNCTION(mod_epoll_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_epoll_shutdown);
AGC_MODULE_DEFINITION(mod_epoll, mod_epoll_load, mod_epoll_shutdown, NULL);

#define MAX_FDSIZE 2048

#define MAX_EPOLLEVENTS 1024

static unsigned int EPOLL_MAX_DISPATCHER = 2;

static volatile int SYSTEM_RUNNING = 0;

static agc_memory_pool_t *module_pool = NULL;

static agc_thread_t **EPOLL_DISPATCH_THREADS = NULL;

static agc_mutex_t **EPOLL_THREADS_MUTEXS = NULL;

static uint8_t *EPOLL_DISPATCH_THREAD_RUNNING = NULL;

static int *EPOLLFDS = NULL;

static int RUNNING_THREAD_COUNT = 0;

static agc_mutex_t *EPOLLSTATE_MUTEX = NULL;

static agc_status_t agc_epoll_add_connection(agc_connection_t *c);

static agc_status_t agc_epoll_del_connection(agc_connection_t *c);

static agc_status_t agc_epoll_add_event(agc_connection_t *c, uint32_t event);

static agc_status_t agc_epoll_del_event(agc_connection_t *c, uint32_t event);

static agc_routine_actions_t agc_epoll_routine = {
    agc_epoll_add_event,
    agc_epoll_del_event,
    agc_epoll_add_connection,
    agc_epoll_del_connection
};

static void agc_epoll_launch_dispatch_threads();

static void *agc_epoll_dispatch_event(agc_thread_t *thread, void *obj);

AGC_MODULE_LOAD_FUNCTION(mod_epoll_load)
{
    module_pool = pool;
    
    *module_interface = agc_loadable_module_create_interface(module_pool, modname);
    
    EPOLL_MAX_DISPATCHER = (agc_core_cpu_count() / 2) + 1;
	if (EPOLL_MAX_DISPATCHER < 2) {
		EPOLL_MAX_DISPATCHER = 2;
	}
    
    agc_mutex_init(&EPOLLSTATE_MUTEX, AGC_MUTEX_NESTED, module_pool);
    
    SYSTEM_RUNNING = 1;
    
    agc_epoll_launch_dispatch_threads();
    
    agc_driver_register_routine(&agc_epoll_routine);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Epoll init success.\n");
    
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_epoll_shutdown)
{
    int x = 0;
    int last = 0;
    
	SYSTEM_RUNNING = 0;
    
    // wait all thread stop
	while (x < 100 && RUNNING_THREAD_COUNT) {
		agc_yield(100000);
		if (RUNNING_THREAD_COUNT == last) {
			x++;
		}
		last = RUNNING_THREAD_COUNT;
	}
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Epoll shutdown success.\n");
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_epoll_add_connection(agc_connection_t *c)
{
	int index = 0;
	struct epoll_event  ee;

	//EPOLLET EPOLLOUT
	ee.events = EPOLLIN|EPOLLRDHUP;
	ee.data.ptr = (void *) (c);
    
	index = agc_random(EPOLL_MAX_DISPATCHER);
    
	//agc_mutex_lock(EPOLL_THREADS_MUTEXS[index]);
	if (epoll_ctl(EPOLLFDS[index], EPOLL_CTL_ADD, c->fd, &ee) == -1) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Epoll add connection failed.\n");
		//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
		return AGC_STATUS_GENERR;
	}

	c->routine->active = 1;
	c->thread_index = index;
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Epoll add connection %d ep_thread %d\n", c->fd, index);
	//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
	return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_epoll_del_connection(agc_connection_t *c)
{
	int  index;
	struct epoll_event  ee;
	 
	assert(c);
	index = c->thread_index;
	ee.events = 0;
	ee.data.ptr = NULL;
    
	//agc_mutex_lock(EPOLL_THREADS_MUTEXS[index]);
	c->routine->active = 0;
	
	if (epoll_ctl(EPOLLFDS[index], EPOLL_CTL_DEL, c->fd, &ee) == -1) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Epoll del connection %d ep_thread %d.\n", c->fd, index);
		//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
		return AGC_STATUS_GENERR;
	}

	//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Epoll remove connection %d\n", c->fd);
	return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_epoll_add_event(agc_connection_t *c, uint32_t event)
{
	int op;
	struct epoll_event  ee;
	uint32_t events, prev;
	int index = 0;

	if (!c || !c->routine) {
		return AGC_STATUS_GENERR;
	}
    
	events = event;

	if (event == AGC_READ_EVENT) {
		prev = EPOLLOUT;
	} else {
		prev = EPOLLIN|EPOLLRDHUP;
	}

	if (c->routine->active) {
		op = EPOLL_CTL_MOD;
		events |= prev;
		index = c->thread_index;
	} else {
		op = EPOLL_CTL_ADD;
		index = agc_random(EPOLL_MAX_DISPATCHER);
	}

	//EPOLLET
	ee.events = events;
	ee.data.ptr = (void *) (c);


	//agc_mutex_lock(EPOLL_THREADS_MUTEXS[index]);
	if (epoll_ctl(EPOLLFDS[index], op, c->fd, &ee) == -1) {
		c->routine->active = 0;
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Epoll add event failed.\n");
		//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
		return AGC_STATUS_GENERR;
	}
	
	c->routine->active = 1;
	//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Epoll connection %d add event %u\n", c->fd, event);

	return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_epoll_del_event(agc_connection_t *c, uint32_t event)
{
	int  op, index;
	uint32_t prev;
	struct epoll_event   ee;

	if (!c || !c->routine) {
		return AGC_STATUS_GENERR;
	}
    	
	index = c->thread_index;
    
	if (event == AGC_READ_EVENT) {
		prev = EPOLLOUT;
	} else {
		prev = EPOLLIN|EPOLLRDHUP;
	}
    
	if (c->routine->active) {
		op = EPOLL_CTL_MOD;
		// | EPOLLET
		ee.events = prev;
		ee.data.ptr = (void *) (c);
	} else {
		return AGC_STATUS_SUCCESS;
	}
    
	//agc_mutex_lock(EPOLL_THREADS_MUTEXS[index]);
	if (epoll_ctl(EPOLLFDS[index], op, c->fd, &ee) == -1) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Epoll del event failed.\n");
		//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);
 		return AGC_STATUS_GENERR;
	}
	//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[index]);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Epoll connection %d del event %u\n", c->fd, event);
    
	return AGC_STATUS_SUCCESS;
}

static void agc_epoll_launch_dispatch_threads()
{
	agc_threadattr_t *thd_attr;
	int index = 0;
	int wait_times = 0;
    
	EPOLLFDS = agc_memory_alloc(module_pool, EPOLL_MAX_DISPATCHER * sizeof(int));
	EPOLL_DISPATCH_THREAD_RUNNING = agc_memory_alloc(module_pool, EPOLL_MAX_DISPATCHER * sizeof(uint8_t));
	EPOLL_DISPATCH_THREADS = agc_memory_alloc(module_pool, EPOLL_MAX_DISPATCHER * sizeof(agc_thread_t *));
	EPOLL_THREADS_MUTEXS = agc_memory_alloc(module_pool, EPOLL_MAX_DISPATCHER * sizeof(agc_mutex_t *));
    
	for (index = 0; index < EPOLL_MAX_DISPATCHER; index++)
	{
		wait_times = 200;
		agc_threadattr_create(&thd_attr, module_pool);
		agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
		agc_threadattr_priority_set(thd_attr, AGC_PRI_REALTIME);
		EPOLLFDS[index] = epoll_create(MAX_FDSIZE);
		agc_mutex_init(&EPOLL_THREADS_MUTEXS[index], AGC_MUTEX_NESTED, module_pool);
		agc_thread_create(&EPOLL_DISPATCH_THREADS[index], thd_attr, agc_epoll_dispatch_event, &EPOLLFDS[index], module_pool);

		while(--wait_times && !EPOLL_DISPATCH_THREAD_RUNNING[index]) {
			agc_yield(10000);
		}

		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Create epoll event dispatch thread %d.\n", index);
    }
}

static void *agc_epoll_dispatch_event(agc_thread_t *thread, void *obj)
{
	int my_id = 0;
	int i = 0;
	int *epollfd_ptr = (int *)obj;
	int epollfd = *epollfd_ptr;
	int ret = 0;
	uint32_t event_flag;
	agc_connection_t *c;
	agc_routine_t *routine = NULL;
	agc_listening_t *listening;
	//agc_event_t *c_event;

	struct epoll_event events[MAX_EPOLLEVENTS];
    
	for (my_id = 0; my_id < EPOLL_MAX_DISPATCHER; my_id++) {
		if (EPOLL_DISPATCH_THREADS[my_id] == thread) {
			break;
		}
	}
    
	agc_mutex_lock(EPOLLSTATE_MUTEX);
	EPOLL_DISPATCH_THREAD_RUNNING[my_id] = 1;
	RUNNING_THREAD_COUNT++;
	agc_mutex_unlock(EPOLLSTATE_MUTEX);
    
	for(;;) {
		if (!SYSTEM_RUNNING)
			break;

		//agc_mutex_lock(EPOLL_THREADS_MUTEXS[my_id]);
		//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d  to wait.\n", my_id);
		ret = epoll_wait(epollfd, events, MAX_EPOLLEVENTS, -1);
		//agc_mutex_unlock(EPOLL_THREADS_MUTEXS[my_id]);

		if (ret == -1 && errno!=EINTR) {			
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "epoll_wait return error.\n");
			break;
		}

		if (ret == 0) {
			//agc_yield(10000);
			continue;
		}
        
		for (i = 0; i < ret; i++) {
			c = events[i].data.ptr;
			event_flag = events[i].events;

			if (c->fd == -1)
				continue;
            
			if (c->listening) {
				listening = c->listening;
				if ((event_flag & EPOLLIN) && listening->handler) {
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d epoll get new socket event of %d.\n", my_id, c->fd);
					listening->handler(c);
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d epoll new socket event of %d finished.\n", my_id, c->fd);
				}
			} else {
				routine = c->routine;
				if (!routine || !routine->active)
					continue;

				//&& routine->err_handle
				if (event_flag & (EPOLLERR|EPOLLHUP)) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "epoll get err event of fd:%d ev:%04XD.\n", c->fd, event_flag);
					event_flag |= EPOLLIN | EPOLLOUT;
					//routine->err_handle(c);
				}
                
				if ((event_flag & EPOLLIN) && routine->read_handle && routine->active) {
					//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d epoll get read event of %d.\n", my_id, c->fd);
					routine->read_handle(c);
					//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d epoll handle read event of %d finished.\n", my_id, c->fd);
				}
                
				if ((event_flag & EPOLLOUT) && routine->write_handle && routine->active) {
					//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d epoll get write event of %d.\n", my_id, c->fd);
					routine->write_handle(c);
					//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "ep_thread %d epoll handle write event of %d finished.\n", my_id, c->fd);
				}
			}
		}
	}
    
	agc_mutex_lock(EPOLLSTATE_MUTEX);
	EPOLL_DISPATCH_THREAD_RUNNING[my_id] = 0;
	RUNNING_THREAD_COUNT--;
	agc_mutex_unlock(EPOLLSTATE_MUTEX);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Epoll dispatch thread %d ended.\n", my_id);
}
