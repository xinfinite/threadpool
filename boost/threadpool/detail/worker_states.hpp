namespace boost{ namespace threadpool{
	enum worker_state{
		worker_null,
		worker_spawn,
		worker_idle,
		worker_working,
		worker_exit,
		worker_exception
	};
}
}