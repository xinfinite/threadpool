#pragma once

namespace boost{ namespace threadpool{ namespace detail{
	template<class TaskScehduler , class Worker>
	class worker_set{
		typedef TaskScheduler scheduler_type;
		typedef Worker worker_type;
		typedef weak_ptr<worker_type> worker_weak_ptr;
		typedef shared_ptr<worker_type> worker_shared_ptr;

		scheduler_type& scheduler_;
		
		mutex leader_mutex_;
		worker_weak_ptr leader_;
		condition_variable leader_condtion_;
	public:
		worker_set(scheduler_type const & s):scheduler_(s){};
		virtual ~worker_set(){};
		void promote_follower(worker_shared_ptr w){
			{
				mutex::scoped_lock lock(leader_mutex_);
				leader_ = w;
			}
			w->promote_to_leader();			
		};
		void promote_follower_if_exist(){
			{
				mutex::scoped_lock lock(leader_mutex_);
				worker_shared_ptr w = 
				leader_ = w;
			}
			w->promote_to_leader();			
		};
		bool has_leader(){
			mutex::scoped_lock lock(leader_mutex_);
			return leader_.lock();
		};
		void wait_for_promotion(worker_shared_ptr w){
			mutex::scoped_lock lock(leader_mutex_);
			while (leader_.lock())
			{
				leader_condtion_.wait(lock);
			}
			leader_ = w;
		};
		void join(worker_shared_ptr w){
			if (has_leader())
			{
				wait_for_promotion();
			}else{

			}						
		};
	};
}}}