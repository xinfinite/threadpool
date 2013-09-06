
#include <boost/threadpool/pool.hpp>
#include <boost/threadpool/detail/pool_core.hpp>

namespace boost{ namespace threadpool{
	fifo_pool::fifo_pool( int initial_threads /*= 0*/ ) : core_( new pool_core_type())
	{
		core_->resize(initial_threads);
	}

	void fifo_pool::schedule( task_type const & task )
	{
		core_->schedule(task);
		return;
	}

	int fifo_pool::total_workers_count() const
	{
		return core_->total_workers_count();
	}

	int fifo_pool::fetching_workers_count() const
	{
		return core_->fetching_workers_count();
	}

	int fifo_pool::processing_workers_count() const
	{
		return core_->processing_workers_count();
	}

	int fifo_pool::pending_tasks_count() const
	{
		return core_->pending_tasks_count();
	}
	
	//不提供取消任务，因为用不到
	/*
	void fifo_pool::clear_pending_tasks()
	{
		return core_->clear_pending_tasks();
	}
	*/

	bool fifo_pool::resize( int const worker_count )
	{
		return core_->resize(worker_count);
	}

	void fifo_pool::wait_for_all_worker_exit() const
	{
		return core_->wait_for_all_worker_exit();
	}

	void fifo_pool::wait_for_all_task_done() const{
		return core_->wait_for_all_task_done();
	}

	void fifo_pool::terminate()
	{
		return core_->terminate();
	}



}}

