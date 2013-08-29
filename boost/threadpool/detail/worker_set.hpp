#pragma once

#include <boost/function.hpp>
//经过研究发现leader follower最大的好处是避免线程同步
//避免内存动态分配是第二好处
//leader/follower适合多事件源的事件分拣（select / WaitForMultiObjects）

namespace boost{ namespace threadpool{ namespace detail{
	template<class Worker>
	class worker_set{
		typedef Worker worker_type;
		typedef weak_ptr<worker_type> worker_weak_ptr;
		typedef shared_ptr<worker_type> worker_shared_ptr;
		typedef worker_type::pool_type pool_type;
		typedef pool_type::task_type task_type;
		
		mutex leader_mutex_;
		worker_weak_ptr leader_;
		condition_variable leader_condtion_;
	public:
		worker_set(){};
		virtual ~worker_set(){};
		
				
		function0<void> join(worker_shared_ptr w){
			{
				mutex::scoped_lock lock(leader_mutex_);
				while (leader_.lock())
				{
					leader_condtion_.wait(lock);
				}
				leader_ = w;
			}
				
			function0<void> t = w->wait_for_task();

			{
				mutex::scoped_lock lock(leader_mutex_);
				leader_.reset();
				leader_condtion_.notify_one();
			}									
		};


	};
}}}