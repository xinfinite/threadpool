/*! \file
* \brief Thread pool core.
*
* This file contains the threadpool's core class: pool<Task, TaskQueuePolicy>.
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




#include <boost/threadpool/detail/locking_ptr.hpp>
#include <boost/threadpool/detail/worker_thread.hpp>
#include <boost/threadpool/task_adaptors.hpp>

#include <boost/thread.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <boost/asio.hpp>

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
    template <typename> class TaskQueuePolicy,
    //template <typename> class SizePolicy,
	template <typename> class SizePolicy,
    template <typename> class SizePolicyController,
    template <typename> class ShutdownPolicy
  > 
  class pool_core
  : public enable_shared_from_this< pool_core<Task, TaskQueuePolicy,SizePolicy, SizePolicyController, ShutdownPolicy > > 
  , private noncopyable
  {
	typedef recursive_mutex	pool_mutex;
	typedef mutex			worker_counting_mutex;
	typedef mutex			event_mutex;
	typedef mutex			task_queue_mutex;
  public: // Type definitions
    typedef Task task_type;                                 //!< Indicates the task's type.
    typedef TaskQueuePolicy<task_type> task_queue_type;     //!< Indicates the scheduler's type.
    typedef pool_core<Task, 
                      TaskQueuePolicy, 
                      SizePolicy,
                      SizePolicyController,
                      ShutdownPolicy > pool_type;           //!< Indicates the thread pool's type.
    //typedef SizePolicy<pool_type> size_policy_type;         //!< Indicates the sizer's type.
	typedef SizePolicy<pool_type> size_policy_type;
    //typedef typename size_policy_type::size_controller size_controller_type;

    typedef SizePolicyController<pool_type> size_controller_type;

//    typedef SizePolicy<pool_type>::size_controller size_controller_type;
    typedef ShutdownPolicy<pool_type> shutdown_policy_type;//!< Indicates the shutdown policy's type.  

    typedef worker_thread<pool_type> worker_type;

	typedef shared_ptr<worker_type> worker_shared_ptr;

	//typedef list<worker_shared_ptr> worker_container;
    // The task is required to be a nullary function.
    BOOST_STATIC_ASSERT(function_traits<task_type()>::arity == 0);

    // The task function's result type is required to be void.
    BOOST_STATIC_ASSERT(is_void<typename result_of<task_type()>::type >::value);


  private:  // Friends 
    friend class worker_thread<pool_type>;

#if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x580)  // Tested with CC: Sun C++ 5.8 Patch 121018-08 2006/12/06
   friend class SizePolicy;
   friend class ShutdownPolicy;
#else
   friend class SizePolicy<pool_type>;
   friend class ShutdownPolicy<pool_type>;
#endif

  private:	
	worker_counting_mutex worker_counting_mutex_;
	size_t m_worker_count;	
	size_t m_target_worker_count;	
	size_t m_active_worker_count;

	size_t worker_fetching_count_;
	size_t worker_processing_count_;	
	
	
  private: // The following members are accessed only by _one_ thread at the same time:
	task_queue_mutex task_queue_mutex_;
	task_queue_type  task_queue_;
	//no ptr need here
    //scoped_ptr<size_policy_type> m_size_policy; // is never null
    size_policy_type m_size_policy;
    bool  m_terminate_all_workers;								// Indicates if termination of all workers was triggered.
    std::vector<shared_ptr<worker_type> > m_terminated_workers; // List of workers which are terminated but not fully destructed.
    
  private: // The following members are implemented thread-safe:
    recursive_mutex  m_monitor;
	
    condition m_worker_idle_or_terminated_event;	// A worker is idle or was terminated.
    //condition m_task_or_terminate_workers_event;  // Task is available OR total worker count should be reduced.

	event_mutex wakeup_worker_mutex_;
	condition wakeup_worker_event_;
	event_mutex worker_exit_mutex_;
	condition worker_exit_event_;
  public:
    /// Constructor.
    pool_core()
      : m_worker_count(0) 
      , m_target_worker_count(0)
      , m_active_worker_count(0)
      , m_terminate_all_workers(false)
    {
      //pool_type volatile & self_ref = *this;
      //m_size_policy.reset(new size_policy_type());

      task_queue_.clear();
    }


    /// Destructor.
    ~pool_core()
    {
    }

    /*! Gets the size controller which manages the number of threads in the pool. 
    * \return The size controller.
    * \see SizePolicy
    */
	
	//do not expose size_policy
	/*size_controller_type size_controller()
	{
	return size_controller_type(*m_size_policy, this->shared_from_this());
	}*/

    /*! Gets the number of threads in the pool.
    * \return The number of threads.
    */
    size_t size() const
    {
      return m_worker_count;
    }

// TODO is only called once
    void shutdown()
    {
      ShutdownPolicy<pool_type>::shutdown(*this);
    }
	    
    void schedule(task_type const & task) volatile
    {
		event_mutex::scoped_lock lock(wakeup_worker_mutex_);
		add_task(task);
		wakeup_worker_event_.notify_one();		
    }	
	
	size_t fetching_workers() const 
	{
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
		return worker_fetching_count_;
	}

    size_t processing_workers() const 
    {
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
		return worker_processing_count_;
    }
	   
    size_t pending_tasks() const 
    {
		task_queue_mutex::scoped_lock lock(task_queue_mutex_);
		return task_queue_.size();		
    }
	   
    void clear_pending_tasks()
    { 
		task_queue_mutex::scoped_lock lock(task_queue_mutex_);
        task_queue.clear();
    }    


    /*! Indicates that there are no tasks pending. 
    * \return true if there are no tasks ready for execution.	
    * \remarks This function is more efficient that the check 'pending() == 0'.
    */   
    bool task_queue_empty() const
    {
      task_queue_mutex::scoped_lock lock(task_queue_mutex_);
      return task_queue_.empty();
    }	


    /*! The current thread of execution is blocked until the sum of all active
    *  and pending tasks is equal or less than a given threshold. 
    * \param task_threshold The maximum number of tasks in pool and scheduler.
    */     
  //  void wait(size_t const task_threshold = 0) const volatile
  //  {
  //    const pool_type* self = const_cast<const pool_type*>(this);
  //    recursive_mutex::scoped_lock lock(self->m_monitor);

  //    if(0 == task_threshold)
  //    {
  //      //while(0 != self->m_active_worker_count || !self->m_scheduler.empty())
		//while( !(self->m_active_worker_count == 0 && self->m_scheduler.empty()) )
  //      { 
  //        self->m_worker_idle_or_terminated_event.wait(lock);
  //      }
  //    }
  //    else
  //    {
  //      while(task_threshold < self->m_active_worker_count + self->m_scheduler.size())
  //      { 
  //        self->m_worker_idle_or_terminated_event.wait(lock);
  //      }
  //    }
  //  }	

    /*! The current thread of execution is blocked until the timestamp is met
    * or the sum of all active and pending tasks is equal or less 
    * than a given threshold.  
    * \param timestamp The time when function returns at the latest.
    * \param task_threshold The maximum number of tasks in pool and scheduler.
    * \return true if the task sum is equal or less than the threshold, false otherwise.
    */       
	/*bool wait(xtime const & timestamp, size_t const task_threshold = 0) const volatile
	{
	const pool_type* self = const_cast<const pool_type*>(this);
	recursive_mutex::scoped_lock lock(self->m_monitor);

	if(0 == task_threshold)
	{
	while(0 != self->m_active_worker_count || !self->m_scheduler.empty())
	{ 
	if(!self->m_worker_idle_or_terminated_event.timed_wait(lock, timestamp)) return false;
	}
	}
	else
	{
	while(task_threshold < self->m_active_worker_count + self->m_scheduler.size())
	{ 
	if(!self->m_worker_idle_or_terminated_event.timed_wait(lock, timestamp)) return false;
	}
	}

	return true;
	}*/


  private:	


    void terminate_all_workers(bool const wait) volatile
    {	
		
      //pool_type* self = const_cast<pool_type*>(this);
      //recursive_mutex::scoped_lock lock(self->m_monitor);

		//keep this line?
		self->m_terminate_all_workers = true;
	  {
		  wakeup_worker_mutex_::scoped_lock lock(wakeup_worker_mutex_);
		  worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
		  m_target_worker_count = 0;
		  wakeup_worker_event_.notify_all();
	  }
	  

      if(wait)
      {
		event_mutex::scoped_lock lock(worker_exit_mutex_);		

        while( fetching_workers() > 0 || processing_workers() > 0)
        {
			worker_exit_event_.wait(lock);
        }

        for(typename std::vector<shared_ptr<worker_type> >::iterator it = self->m_terminated_workers.begin();
          it != self->m_terminated_workers.end();
          ++it)
        {
          (*it)->join();
        }
        self->m_terminated_workers.clear();
      }
    }


    /*! Changes the number of worker threads in the pool. The resizing 
    *  is handled by the SizePolicy.
    * \param threads The new number of worker threads.
    * \return true, if pool will be resized and false if not. 
    */
    bool resize(size_t const worker_count) volatile
    {
      locking_ptr<pool_type, recursive_mutex> lockedThis(*this, m_monitor); 

      if(!m_terminate_all_workers)
      {
        m_target_worker_count = worker_count;
      }
      else
      { 
        return false;
      }
	  int worker_needed = worker_adjust_amount();
      if( worker_needed > 0)
      { // increase worker count
        while(worker_adjust_amount() > 0)
        {
          try
          {
            worker_thread<pool_type>::create_and_attach(lockedThis->shared_from_this());
            m_worker_count++;
            m_active_worker_count++;	
          }
          catch(thread_resource_error)
          {
            return false;
          }
        }
      }
      else if( worker_needed < 0)
      { // decrease worker count
			lockedThis->m_task_or_terminate_workers_event.notify_all();   // TODO: Optimize number of notified workers
      }

      return true;
    }

	int worker_adjust_amount() 
	{		
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
		int target = m_target_worker_count;
		target -= worker_fetching_count_;
		target -= worker_processing_count_;
		
		return target;
	}


    // worker died with unhandled exception
    void worker_died_unexpectedly(shared_ptr<worker_type> worker) volatile
    {
      //locking_ptr<pool_type, recursive_mutex> lockedThis(*this, m_monitor);
	  
	  
	  
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);
		m_worker_count--;
		m_active_worker_count--;	  
      
		m_worker_idle_or_terminated_event.notify_all();	

		if(m_terminate_all_workers)
		{
			//ÓÐ±ØÒªÂð£¿
			m_terminated_workers.push_back(worker);
		}
		else
		{
			m_size_policy->worker_died_unexpectedly(m_worker_count);
		}
    }

    void worker_destructed(shared_ptr<worker_type> worker) volatile
    {
		//locking_ptr<pool_type, recursive_mutex> lockedThis(*this, m_monitor);

      m_worker_count--;
      m_active_worker_count--;
      lockedThis->m_worker_idle_or_terminated_event.notify_all();	

      if(m_terminate_all_workers)
      {
        lockedThis->m_terminated_workers.push_back(worker);
      }
    }
	
	void worker_fetching(){
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		
		worker_processing_count_--;
		worker_fetching_count_++;
	};
	void worker_processing(){
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		
		worker_fetching_count_--;
		worker_processing_count_++;
	};
	void worker_exit(){
		event_mutex::scoped_lock awake_lock(worker_exit_mutex_);
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		
		worker_fetching_count_--;				
		worker_exit_event_.notify_one();
	};
	void worker_exception(){		
		worker_counting_mutex::scoped_lock lock(worker_counting_mutex_);		
		worker_processing_count_--;
	};

	//operation on task queue	
	bool fetch_task(Task & task){
		task_queue_mutex::scoped_lock lock(task_queue_mutex_);
		if(s.size()){		  
			task = task_queue_.top();
			task_queue_.pop();
			return true;
		}else{
			return false;
		}
	};
	
	void add_task(Task& t){
		task_queue_mutex::scoped_lock lock(task_queue_mutex_);
		task_queue_.push(t);
	};
	
    void execute_task()
    {
		while(true){
			function0<void> task;	
			worker_fetching();
			{			
				event_mutex::scoped_lock awake_lock(wakeup_worker_mutex_);

				while(worker_adjust_amount() >= 0 && !fetch_task(task));
				{				
					wakeup_worker_event_.wait(awake_lock);
				}
			}	

			if(task)
			{
				scope_guard guard(bind(&pool_type::worker_exception,shared_from_this()));
				worker_processing();
				task();
				guard.disable();				
			}else{
				worker_exit();
				return;
			}			
		}
		return;
    }
};




} } } // namespace boost::threadpool::detail

#endif // THREADPOOL_POOL_CORE_HPP_INCLUDED
