#include <boost/thread/mutex.hpp>
#include <threadpool/worker_state.hpp>
namespace boost { namespace threadpool { namespace detail {
	
	typedef int sizing_decision;
	
	template< int N >
	class SizingPolicy{
		mutex worker_mutex;
		int worker_spawned;		
	public:
		SizingPolicy():worker_spawned(0){
			
		};
		virtual ~SizingPolicy(){
		
		};						
		sizing_decision on_task_state(const task_state& last_state, const task_state& state){
			return 0;
		};
		sizing_decision on_worker_state(const worker_state& last_state , const worker_state & state){
			if(last_state == worker_null && state == worker_spawn){
				mutex::scoped_lock lock(worker_mutex);
				worker_spawned++;
			}
			if(state == worker_exit){
				mutex::scoped_lock lock(worker_mutex);
				worker_spawned--;
			}
			if(worker_spawned < N){
				return 1;
			}else if(worker_spawned > N){
				return -1;
			}
			return 0;
		};				
	};
	
	template< int Min , int Max >
	class RangeSizingPolicy{
		mutex worker_mutex;
		int worker_spawned;		
	public:
		RangeSizingPolicy():worker_spawned(0),worker_idle(0){
			
		};
		virtual ~RangeSizingPolicy(){
		
		};						
		sizing_decision on_task_state(const task_state& last_ts, const task_state& ts){
			return 0;
		};
		sizing_decision on_worker_state(const worker_state& last_state , const worker_state & state){
			if(last_state == worker_null && state == worker_spawn){
				mutex::scoped_lock lock(worker_mutex);
				worker_spawned++;
			}
			if(state == worker_exit){
				mutex::scoped_lock lock(worker_mutex);
				worker_spawned--;
			}
			
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