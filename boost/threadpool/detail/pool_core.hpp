/*! \file
* \brief Thread pool core.
*
* This file contains the threadpool's core class: pool<Task, QueuePolicy>.
*
* Thread pools are a mechanism for asynchronous and parallel processing 
* within the same process. The pool class provides a convenient way 
* for dispatching asynchronous tasks as functions objects. The scheduling
* of these tasks can be easily controlled by using customized schedulers. 
*
* Copyright (c) 2005-2007 Philipp Henkel
*
* Use, modification, and distribution are  subject to the
* Boost Software License, Version 1.0. (See accompanying  file
* LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*
* http://threadpool.sourceforge.net
*
*/


#ifndef THREADPOOL_POOL_CORE_HPP_INCLUDED
#define THREADPOOL_POOL_CORE_HPP_INCLUDED
#include <boost/threadpool/pool.hpp>
#include <boost/threadpool/detail/worker_thread.hpp>
#include <boost/thread.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/result_of.hpp>
//#include <boost/thread/reverse_lock.hpp>

#include <vector>



/// The namespace threadpool contains a thread pool and related utility classes.
namespace boost { namespace threadpool { namespace detail 
{

	/*! \brief Thread pool. 
	*
	* Thread pools are a mechanism for asynchronous and parallel processing 
	* within the same process. The pool class provides a convenient way 
	* for dispatching asynchronous tasks as functions objects. The scheduling
	* of these tasks can be easily controlled by using customized schedulers. 
	* A task must not throw an exception.
	*
	* A pool_impl is DefaultConstructible and NonCopyable.
	*
	* \param Task A function object which implements the operator 'void operator() (void) const'. The operator () is called by the pool to execute the task. Exceptions are ignored.
	* \param Scheduler A task container which determines how tasks are scheduled. It is guaranteed that this container is accessed only by one thread at a time. The scheduler shall not throw exceptions.
	*
	* \remarks The pool class is thread-safe.
	* 
	* \see Tasks: task_func, prio_task_func
	* \see Scheduling policies: fifo_scheduler, lifo_scheduler, prio_scheduler
	*/ 
	template <
		typename Task, 	
		template <typename> class QueuePolicy	
	> 
	class pool_core
		: public enable_shared_from_this< pool_core<Task, QueuePolicy > > 
		, private noncopyable
	{
		typedef recursive_mutex	pool_mutex;
		typedef recursive_mutex	worker_counting_mutex;
		typedef mutex			event_mutex;
		typedef recursive_mutex	task_queue_mutex;
		typedef mutex			resize_mutex;
	public: // Type definitions
		typedef Task task_type;                                 //!< Indicates the task's type.
		
		typedef pool_core< Task, QueuePolicy > pool_type;           //!< Indicates the thread pool's type.

		typedef shared_ptr<pool_type> ptr_type;					//!< Indicates the pool's ptr type

		typedef QueuePolicy<task_type> queue_policy_type;     //!< Indicates the queue policy's type.
		
		typedef worker_thread<pool_type> worker_type;
			
		// The task is required to be a nullary function.
		BOOST_STATIC_ASSERT(function_traits<task_type()>::arity == 0);

		// The task function's result type is required to be void.
		BOOST_STATIC_ASSERT(is_void<typename result_of<task_type()>::type>::value);


	private:  // Friends 
		friend class worker_thread<pool_type>;

	private:	
		mutable worker_counting_mutex	worker_counting_mutex_;			//protects follow counters
		mutable condition_variable_any worker_counting_event_;			//signals when follow counters changed

		int target_worker_count_;			// target worker count
		int fetching_workers_count_;			// count workers in fetching state
		int processing_workers_count_;		// count workers in processing state
		resize_mutex resize_mutex_;	//! make sure one resize call each time

	private: 
		mutable task_queue_mutex task_queue_mutex_;		//protects task_queue
		queue_policy_type  task_queue_;			//task queue policy object
		condition_variable task_queue_changed_event_;

	private:

		//!
		mutable event_mutex worker_mutex_;
		mutable condition_variable worker_state_changed_event_;
		mutable condition_variable_any worker_fetch_one_event_;		 
		condition_variable_any worker_enter_event_;			
		condition_variable_any worker_exit_on_request_event_;
		condition_variable_any worker_exit_on_exception_event_;
	public:
		/// Constructor.
		pool_core()
			: target_worker_count_(0)
			, fetching_workers_count_(0)
			, processing_workers_count_(0)			
		{
			//pool_type volatile & self_ref = *this;
			//m_size_policy.reset(new size_policy_type());
			
			task_queue_.clear();
		}


		/// Destructor.
		~pool_core()
		{
			clear_pending_tasks();
			//terminate();	//如果析构函数被调用，说明worker和user都不再保存pool_core的shared_ptr
		}
		
		void schedule(task_type const & task)
		{
			event_mutex::scoped_lock lock(worker_mutex_);
			add_task(task);
			worker_fetch_one_event_.notify_one();		
		}	
		int total_workers_count() const{
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
			return fetching_workers_count_ + processing_workers_count_;
		}

		int fetching_workers_count() const 
		{
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
			return fetching_workers_count_;
		}

		int processing_workers_count() const 
		{
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
			return processing_workers_count_;
		}

		int pending_tasks_count() const 
		{
			task_queue_mutex::scoped_lock lock(task_queue_mutex_);
			return task_queue_.size();		
		}

   




		bool resize(int const worker_count)
		{
			resize_mutex::scoped_try_lock resize_lock(resize_mutex_);
			if (!resize_lock)
			{
				return false;
			}		

			//event_mutex::scoped_lock lock(worker_mutex_);

			//worker_counting_mutex::scoped_lock counting_lock(worker_counting_mutex_);					
			//event_mutex::scoped_lock lock(worker_mutex_);

			//target_worker_count_ is protected as a condition
			

			int worker_adjust = worker_adjust_amount(worker_count);		

			if(worker_adjust > 0){
				event_mutex::scoped_lock lock(worker_mutex_);

				set_target_worker_count(worker_count);

				//TODO wait after each create_and_attach
				while(worker_adjust_amount(worker_count) > 0)
				{			
					try
					{
						worker_thread<pool_type>::create_and_attach(shared_from_this());
					}
					catch(thread_resource_error)
					{
						return false;
					}					
					worker_enter_event_.wait(lock);
					//worker_adjust--;
				}
			}else if(worker_adjust < 0){
				event_mutex::scoped_lock lock(worker_mutex_);										

				while (worker_adjust_amount(worker_count) < 0)
				{				

					int last_total_worker = total_workers_count();
					{		

						//last_total_worker = total_workers_count();	//!FIXME 如果此时有worker异常退出，有可能会多停一个工作者！

						set_target_worker_count(last_total_worker - 1);					

						worker_fetch_one_event_.notify_one();	

						while(total_workers_count() >= last_total_worker){
							worker_exit_on_request_event_.wait(lock);
						}
					}					
					
					/*{
						worker_counting_mutex::scoped_lock count_lock(worker_counting_mutex_);
						
						while (last_total_worker < total_workers_count() )
						{
							worker_exit_on_request_event_.wait(count_lock);
						}
						
					}*/
					
					//worker_adjust++;
				}
			}	
			
			
			//{
			//	worker_counting_mutex::scoped_lock counting_lock(worker_counting_mutex_);					

			//	//TODO wait after each notify
			//	while (worker_adjust_amount(worker_count))
			//	{
			//		worker_counting_event_.wait(counting_lock);
			//	}
			//	return true;
			//}
		}

		//wait_for_all_worker_exit for all worker to exit from fetching or processing state
		void wait_for_all_worker_exit() const{
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);

			while (total_workers_count() > 0)
			{
				worker_counting_event_.wait(lock);
			}
		};
		void wait_for_all_task_done() const {
			/*event_mutex::scoped_lock evt_lock(worker_mutex_);
			while(pending_tasks_count() > 0 || processing_workers_count() > 0 ){
				if(total_workers_count() == 0){
					throw no_worker();
				}
				worker_.wait(evt_lock);				
			}*/
			
			{
				//worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
				event_mutex::scoped_lock lock(worker_mutex_);
				
				while(pending_tasks_count() > 0){
					if(total_workers_count() == 0){
						throw no_worker();
					}
					worker_counting_event_.wait(lock);
				}
			}		

		}

		//! \brief notify all workers to exit
		//! The terminate returns immediately , worker could be still running after terminate returns.		
		void terminate()
		{	
			resize_mutex::scoped_lock resize_lock(resize_mutex_);
			
			{//set target worker count to 0 and notify all workers to exit
				event_mutex::scoped_lock lock(worker_mutex_);
				set_target_worker_count(0);
				worker_fetch_one_event_.notify_all();
			}

			/*
			if(wait)
			{
				worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		

				while( fetching_workers_count() > 0 || processing_workers_count() > 0)
				{
					worker_counting_event_.wait(lock);
				}				
			}
			*/
		}
	private:
		/*!
		\brief empty the task queue


		*/
		void clear_pending_tasks()
		{ 
			task_queue_mutex::scoped_lock lock(task_queue_mutex_);
			task_queue_.clear();
		} 

		/*! 
		\brief check if task queue is empty
				
		\return true if there are no tasks ready for execution.	
		\remarks This function is more efficient that the check 'pending() == 0'.
		*/   
		bool task_queue_empty() const
		{
			task_queue_mutex::scoped_lock lock(task_queue_mutex_);
			return task_queue_.empty();
		}	
		
		//! \brief set target worker count with signal
		//! use by pool_core::resize and pool_core::terminate to update target worker count
		void set_target_worker_count(int target)
		{
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
			target_worker_count_ = target;
			worker_counting_event_.notify_all();
		}
		//! \brief caculate how many worker should be terminated or spawned
		//
		int worker_adjust_amount(int target) 
		{		
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);			
			target -= fetching_workers_count_;
			target -= processing_workers_count_;

			return target;
		}
		
		//! \brief update counters and emit signal
		void worker_begin_fetching(){
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);				
			fetching_workers_count_++;
			worker_counting_event_.notify_all();			
		};
		//! \brief update counters and emit signal
		void worker_fetching_to_processing(){
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);				
			fetching_workers_count_--;
			processing_workers_count_++;
			worker_counting_event_.notify_all();
		};
		//! \brief update counters and emit signal
		void worker_processing_to_fetching(){
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);				
			processing_workers_count_--;
			fetching_workers_count_++;
			worker_counting_event_.notify_all();
		};
		//! \brief update counters and emit signal
		void worker_fetching_to_exit(){			
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		
			fetching_workers_count_--;				
			worker_counting_event_.notify_all();
			
		};
		//! \brief update counters and emit signal
		void worker_processing_to_exception(){		
			event_mutex::scoped_lock evt_lock(worker_mutex_);
			worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		
			processing_workers_count_--;
			worker_counting_event_.notify_all();
			worker_exit_on_exception_event_.notify_all();
			//worker_state_changed_event_.notify_all();
		};

		//! \brief fetch a task from task queue policy object
		//! returns false immediately if the queue is empty.
		//! otherwise it will return true , indicating the
		//! Task & task is valid. This method is thread-safe.		
		bool fetch_task(Task & task){
			task_queue_mutex::scoped_lock lock(task_queue_mutex_);
			if(task_queue_.size()){		  
				task = task_queue_.top();
				task_queue_.pop();
				task_queue_changed_event_.notify_all();
				return true;
			}else{
				return false;
			}
		};

		//! \brief add a task into task queue policy
		//! This method is thread-safe.
		void add_task(Task const& t){
			task_queue_mutex::scoped_lock lock(task_queue_mutex_);
			task_queue_.push(t);
			task_queue_changed_event_.notify_all();
		};

		//! \brief entry method for worker
		//! ThreadSafety : yes
		void execute_task()
		{
			{
				event_mutex::scoped_lock evt_lock(worker_mutex_);
				worker_begin_fetching();
				worker_enter_event_.notify_all();
				worker_state_changed_event_.notify_all();
			}
			
			bool from_processing = false;

			while(true){
				function0<void> task;	

				{			
					event_mutex::scoped_lock awake_lock(worker_mutex_);
					
					//fetching state
					if(from_processing){
						worker_processing_to_fetching();
						worker_state_changed_event_.notify_all();
					}else{
						from_processing = true;
					}					

					while(worker_adjust_amount(target_worker_count_) >= 0 && !fetch_task(task))
					{						
						worker_fetch_one_event_.wait(awake_lock);
					}

					if()
					if(!task){
						worker_fetching_to_exit();
						worker_exit_on_request_event_.notify_all();
						//worker_state_changed_event_.notify_all();
						return;
					}else{
						worker_fetching_to_processing();
						//worker_state_changed_event_.notify_all();
					}
					
				}	

				if(task)
				{					
					scope_guard guard(bind(&pool_type::worker_processing_to_exception,shared_from_this()));				
					task();
					guard.disable();
					
				}			
			}
			return;
		};


	};




} } } // namespace boost::threadpool::detail

#endif // THREADPOOL_POOL_CORE_HPP_INCLUDED
