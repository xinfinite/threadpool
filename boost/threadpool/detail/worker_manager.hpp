#ifndef THREADPOOL_SIZE_POLICIES_HPP_INCLUDED
#define THREADPOOL_SIZE_POLICIES_HPP_INCLUDED



/// The namespace threadpool contains a thread pool and related utility classes.
namespace boost { namespace threadpool { namespace detail
{

	class worker_manager{
	public:
		virtual void on_worker_sleep();
		virtual void on_worker_wakeup();
		virtual void wakeup_worker();
		
		virtual void new_worker();
		virtual void terminate_worker();
		
		virtual int count_workers();
		virtual int count_idle_workers();

	};
	class resizable_sizing_policy : public worker_manager{
	public:
		void on_new_task();		
		void on_cancel_task();

		void on_task_done();
		void on_task_exception();

		void on_worker_sleep();
		void on_worker_wakeup();

	};
  

} } }// namespace boost::threadpool

#endif // THREADPOOL_SIZE_POLICIES_HPP_INCLUDED
