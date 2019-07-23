#pragma once
#include <future>
#include <thread>
#include <vector>

namespace dev
{
namespace brc
{
template <typename T, typename R>
struct Task
{
	typedef  std::vector<T>     value_source;
	typedef  std::vector<R>     value_result;

	Task(uint32_t numbers = 4) :m_thread(numbers)
	{

	}
	template <typename CONVERT>
	void go_task(const value_source &source, value_result &result, CONVERT conver_func)
	{
		std::vector<std::vector<T>>  split_source(m_thread);
		std::vector<std::vector<R>>  split_result(m_thread);

		auto one_size = source.size() / m_thread;
		auto sp_size = source.size() % one_size;        //find surplus
		std::vector<std::future<value_result>> vec_ff;


		for(int i = 0; i < m_thread; i++)
		{
			if(i == m_thread - 1 && sp_size != 0)
			{  //add surplus to last thread
				split_source[i].assign(source.begin() + i * one_size, source.begin() + (i + 1) * one_size + sp_size);
			}
			else
			{
				split_source[i].assign(source.begin() + i * one_size, source.begin() + (i + 1) * one_size);
			}
			std::packaged_task<value_result(value_source)>  task([&](const value_source &d){
				value_result ret;
				for(auto itr : d)
				{
					ret.push_back(conver_func(itr));
				}
				return ret;
																 });
			vec_ff.push_back(task.get_future());
			std::thread(std::move(task), split_source[i]).detach();
		}


		for(int i = 0; i < m_thread; i++)
		{
			vec_ff[i].wait();
			split_result[i] = vec_ff[i].get();
		}

		for(auto i = 0; i < m_thread; i++)
		{
			result.insert(result.end(), split_result[i].begin(), split_result[i].end());
		}
	}
private:
	uint32_t  m_thread;
};
}
}