#include <boost/thread/mutex.hpp>
#include <threadpool/worker_state.hpp>
namespace boost { namespace threadpool { namespace detail {
	
	typedef int sizing_decision;
	
	
	
	class SizingPolicy{
	protected:
		mutex worker_mutex;
		int worker_spawned;		
		int worker_idle;

		mutex task_mutex;
		int task_pending;
		int task_running;

	public:
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
		
		sizing_decision on_worker_spawn(){
			mutex::scoped_lock lock(worker_mutex);
			worker_spawned++;
			return make_decision();
		};

		sizing_decision on_worker_idle(){
			mutex::scoped_lock lock(worker_mutex);
			worker_idle++;
			return make_decision();
		};

		sizing_decision on_worker_working(){
			mutex::scoped_lock lock(worker_mutex);
			worker_idle--;
			return make_decision();
		};

		sizing_decision on_worker_exit(){
			mutex::scoped_lock lock(worker_mutex);
			worker_spawned--;
			return make_decision();
		};

		virtual sizing_decision make_decision() = 0;
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