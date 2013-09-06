// threadpool.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <boost/threadpool.hpp>

using namespace boost::threadpool;
#include <windows.h>

#include <gtest/gtest.h>

#include <gtest/test1.hpp>
#include <gtest/test2.hpp>
#include <gtest/test3.hpp>

void simple_task(){
	static int i = 0;
	i++;
	Sleep(10);
	return;
};
int _tmain(int argc, _TCHAR* argv[])
{
	//  ::testing::FLAGS_gtest_repeat = 1000;
	// 	::testing::FLAGS_gtest_output = "xml";

	::testing::InitGoogleTest(&argc, argv);

	int r = RUN_ALL_TESTS();
	
	system("pause");
	return r;
	//return 0;
}
