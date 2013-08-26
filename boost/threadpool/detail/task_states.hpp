namespace boost{ namespace threadpool{
	enum task_state{
		task_null,
		task_pending,
		task_cancelled,
		task_executing,
		task_releasing		
	};
}
}