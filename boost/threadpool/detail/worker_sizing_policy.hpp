#include <boost/thread/mutex.hpp>

namespace boost { namespace threadpool { namespace detail {
	
	
	
	
	
	class SizingPolicy{
	protected:
		mutex worker_mutex;
		int worker_spawned;		
		int worker_idle;
		int worker_working;

		mutex task_mutex;
		int task_pending;
		int task_running;

	public:
		/// \brief Decision to create or release worker
		/// 0 means no change
		/// 1 means one more
		/// -1 means one less
		typedef int sizing_decision;

		SizingPolicy():worker_spawned(0){
			
		};
		virtual ~SizingPolicy(){
		
		};						
		sizing_decision on_task_schedule(){
			mutex::scoped_lock lock(task_mutex);
			task_pending++;
			return make_decision();
		};
		sizing_decision on_task_run(){
			mutex::scoped_lock lock(task_mutex);
			task_pending--;
			task_running++;
			return make_decision();
		};
		sizing_decision on_task_finish(){
			mutex::scoped_lock lock(task_mutex);			
			task_running--;
			return make_decision();
		};
		sizing_decision on_task_cancel(){
			mutex::scoped_lock lock(task_mutex);
			task_pending--;
			
			return make_decision();
		};	
		
		//sizing_decision on_worker_spawn(){
		//	mutex::scoped_lock lock(worker_mutex);
		//	worker_spawned++;
		//	return make_decision();
		//};

		sizing_decision on_worker_ready(){
			mutex::scoped_lock lock(worker_mutex);
			worker_idle++;
			return make_decision();
		};
		sizing_decision on_worker_finished(){
			mutex::scoped_lock lock(worker_mutex);
			worker_idle++;
			worker_working--;
			return make_decision();
		};

		sizing_decision on_worker_working(){
			mutex::scoped_lock lock(worker_mutex);
			worker_idle--;
			worker_working++;
			return make_decision();
		};

		//sizing_decision on_worker_exit(){
		//	mutex::scoped_lock lock(worker_mutex);
		//	worker_spawned--;
		//	return make_decision();
		//};

		virtual sizing_decision make_decision() = 0;

		int count_working_worker(){
			mutex::scoped_lock lock(worker_mutex);
			return worker_working;
		};
		int count_idle_worker(){
			mutex::scoped_lock lock(worker_mutex);
			return worker_idle;
		};
		int count_worker(){
			mutex::scoped_lock lock(worker_mutex);
			return worker_idle + worker_working;
		};

		int count_running_task(){
			mutex::scoped_lock lock(task_mutex);
			return task_running;
		};

		int count_pending_task(){
			mutex::scoped_lock lock(task_mutex);
			return task_pending;
		};

		int count_task(){
			mutex::scoped_lock lock(task_mutex);
			return task_pending+task_running;
		};
	};

	template<int N>
	class FixedNumberSizingPolicy : private SizingPolicy{
	public:
		sizing_decision make_decision(){			
			if(worker_spawned < N){
				return 1;
			}else if(worker_spawned > N){
				return -1;
			}
			return 0;
		};
	};
	
	template< int Min , int Max >
	class RangeSizingPolicy : private SizingPolicy{
	public:
		sizing_decision make_decision(){			
			if(worker_spawned < Min){
				return 1;
			}else if(worker_spawned > Max){
				return -1;
			}
			return 0;
		};
	}
	
	
	
}
}
}