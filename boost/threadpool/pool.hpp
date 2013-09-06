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


#ifndef THREADPOOL_POOL_HPP_INCLUDED
#define THREADPOOL_POOL_HPP_INCLUDED

#include <boost/ref.hpp>
#include <boost/utility.hpp>
#include <boost/threadpool/task_adaptors.hpp>
//#include <boost/threadpool/detail/pool_core.hpp>	//this is hidden as pimpl requires
#include <boost/threadpool/scheduling_policies.hpp>

/// The namespace threadpool contains a thread pool and related utility classes.
namespace boost { namespace threadpool
{	

	namespace detail{
		template<
			class,
			template <class> class
		> class pool_core;
	};

	typedef detail::pool_core<task_func, lifo_scheduler> lifo_pool_core;
	typedef detail::pool_core<prio_task_func, prio_scheduler> prio_pool_core;

	
	typedef shared_ptr<lifo_pool_core> lifo_pool_core_ptr;
	typedef shared_ptr<prio_pool_core> prio_pool_core_ptr;

	template <class PoolCore>
	shared_ptr<PoolCore> make_pool(){
		return shared_ptr<PoolCore>(new PoolCore());
	};

	struct no_worker{};
  class fifo_pool   
  {
  public: // Type definitions
	  typedef task_func task_type;
  private:
    typedef detail::pool_core<task_func, fifo_scheduler> pool_core_type;
	typedef shared_ptr<pool_core_type> pool_core_ptr_type;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
    pool_core_ptr_type          core_; // pimpl idiom   

	
  

  public:
    
    fifo_pool(int initial_threads = 0);
	  
	void schedule(task_type const & task);
    
    int total_workers_count() const;

	int fetching_workers_count() const;

	int processing_workers_count() const;

	int pending_tasks_count() const;
   
	//”√≤ªµΩ
	/*void clear_pending_tasks();*/

	//!if calling resize from two thread , the latter one will fail
	bool resize(int const worker_count);
	
	void wait_for_all_worker_exit() const;	

	void wait_for_all_task_done() const;

	void terminate();   
  };

} } // namespace boost::threadpool

#endif // THREADPOOL_POOL_HPP_INCLUDED
