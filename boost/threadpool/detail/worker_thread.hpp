/*! \file
* \brief Thread pool worker.
*
* The worker thread instance is attached to a pool 
* and executes tasks of this pool. 
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

#ifndef THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED
#define THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED


#include <boost/threadpool/detail/scope_guard.hpp>

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>



namespace boost { namespace threadpool { namespace detail 
{
  /*! \brief Thread pool worker. 
  *
  * A worker_thread represents a thread of execution. The worker is attached to a 
  * thread pool and processes tasks of that pool. The lifetime of the worker and its 
  * internal boost::thread is managed automatically.
  *
  * This class is a helper class and cannot be constructed or accessed directly.
  * 
  * \see pool_core
  */ 
  template <typename Pool>
  class worker_thread
  : public enable_shared_from_this< worker_thread<Pool> > 
  , private noncopyable
	{
		typedef condition_variable condition;
	public:
		typedef Pool pool_type;         	   //!< Indicates the pool's type.

		typedef typename pool_type::ptr_type pool_ptr;
		
		typedef shared_ptr<worker_thread> ptr_type;
	private:
		typename pool_type::ptr_type      m_pool;     //!< Pointer to the pool which created the worker.

		boost::thread  thread_;   //!< Pointer to the thread which executes the run loop.
		
		
    
    /*! Constructs a new worker. 
    * \param pool Pointer to it's parent pool.
    * \see function create_and_attach
    */
	worker_thread(shared_ptr<pool_type> & pool)	: m_pool(pool)
	{
		assert(pool);		
	}

  public:
	  /*! Executes pool's tasks sequentially.
	  */
	  
	  void run(){
		  m_pool->execute_task();		  
	  }
	
	  /*! Joins the worker's thread.
	  */
	  void join()
	  {
		  thread_->join();
	  }
	  	
	  /*! Constructs a new worker thread and attaches it to the pool.
	  * \param pool Pointer to the pool.
	  */

	  
	  static ptr_type create_and_attach(shared_ptr<pool_type> pool)
	  {
		  ptr_type worker(new worker_thread(pool));
		  if(!worker)
		  {		 
			  throw std::bad_alloc("boost::thread_pool::detail::worker_thread");
		  }

		  worker->thread_ = boost::thread(bind(&worker_thread::run, worker));

		  return worker;
	  };

  };


} } } // namespace boost::threadpool::detail

#endif // THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED

