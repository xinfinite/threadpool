#pragma once

#include <boost/threadpool.hpp>

#include <gtest/gtest.h>

#include <boost/thread.hpp>

#include <boost/chrono.hpp>
//this file contains test cases for fifo pool

using namespace boost::this_thread;	
using namespace boost::posix_time;

class test1 : public ::testing::Test
{	
public:		
	boost::mutex m;
	
	fifo_pool p1;
	fifo_pool p2;

	virtual void SetUp() {
		test_task_called_counter = 0;
		p1.resize(4);
		p2.resize(2);
	}

	virtual void TearDown(){
		p1.terminate();
		p2.terminate();
	}
	volatile int barrier;
	volatile int test_task_called_counter;

	void test_task(){
		boost::mutex::scoped_lock lock(m);
		test_task_called_counter++;		
	};

	void test_task_10ms(){
		sleep(milliseconds(10));
	};

	
};

TEST_F(test1 , basicUsage){
	task_func t(boost::bind(&test1::test_task,this));

	p1.schedule(t);
	p2.schedule(t);

	p1.wait_for_all_task_done();
	p2.wait_for_all_task_done();

	int x = barrier;
	EXPECT_EQ(2,test_task_called_counter);
};

TEST_F(test1 , resize1000Times){
	task_func t(boost::bind(&test1::test_task,this));

	int i = 1000;
	while (i--)
	{
		p1.resize(5);
		EXPECT_EQ(5,p1.fetching_workers_count());
		EXPECT_EQ(0,p1.processing_workers_count());

		p2.resize(5);
		EXPECT_EQ(5,p2.fetching_workers_count());
		EXPECT_EQ(0,p2.processing_workers_count());

		p1.resize(1);
		EXPECT_EQ(1,p1.total_workers_count());

		p2.resize(1);
		EXPECT_EQ(1,p2.total_workers_count());
	}
	
}

TEST_F(test1 , resize1000TimesWithTaskSchedule){
	task_func t(boost::bind(&test1::test_task,this));

	int i = 1000;
	while (i--)
	{
		p1.resize(5);
		EXPECT_EQ(5,p1.total_workers_count());

		p2.resize(5);
		EXPECT_EQ(5,p2.total_workers_count());

		for(int j = 0 ; j  < 10 ; j++){
			p1.schedule(t);
			p2.schedule(t);
		}

		p1.resize(1);	//this will not cancel the task , it just pend in the task queue
						//when we resize the pool to 5 , it will immediately got executed.
						//so , the following EXPECT_EQ could fail
		EXPECT_EQ(1,p1.total_workers_count());

		p2.resize(1);
		EXPECT_EQ(1,p2.total_workers_count());		
	}

	p1.wait_for_all_task_done();
	p2.wait_for_all_task_done();

	EXPECT_EQ(20000,test_task_called_counter);

}
																																																																																																																																																																																																																																																																																																	
using namespace boost::chrono;

TEST_F(test1,taskExecuteSpeed){
	
	p1.resize(10);

	task_func t(boost::bind(&test1::test_task_10ms,this));

	const int loop = 1000;
	int i = loop;
	
	time_point<system_clock> begin_time = system_clock::now();

	while(i--){
		p1.schedule(t);
	}

	p1.wait_for_all_task_done();

	boost::chrono::milliseconds ms = duration_cast<boost::chrono::milliseconds>(system_clock::now() - begin_time);	

	EXPECT_LT(ms , boost::chrono::milliseconds((loop * 10 / 10) + (loop * 1)));
}