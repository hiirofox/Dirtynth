#pragma once

#ifdef _WIN32
#include <windows.h>
#elif
#include <pthread.h>
#include <sched.h>
#endif

namespace Dirtynth
{
	class Platform
	{
	public:
		constexpr static int NumCores = 4;//t153

		static void BindToCPU(int cpu)//绑核。对于比较弱的cpu尤其重要。防止系统调度把mutant计算插到实时线程造成抖动
		{
#ifdef _WIN32
			//windows一般比较强，不如不绑
			//HANDLE thread = GetCurrentThread();
			//DWORD_PTR mask = (DWORD_PTR(1) << cpu);
			//SetThreadAffinityMask(thread, mask);
#elif
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(cpu, &cpuset);
			pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif
		}
	};
}