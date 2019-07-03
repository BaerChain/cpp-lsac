#pragma once
#include <utility>
#include <queue>
#include <condition_variable>
#include <mutex>


namespace dev
{

/// Concurrent queue.
/// You can push and pop elements to/from the queue. Pop will block until the queue is not empty.
/// The default backend (_QueueT) is std::queue. It can be changed to any type that has
/// proper push(), pop(), empty() and front() methods.
template<typename _T, typename _QueueT = std::queue<_T>>
class concurrent_queue
{
public:
	template<typename _U>
	void push(_U&& _elem)
	{
		{
			std::lock_guard<decltype(x_mutex)> guard{x_mutex};
			m_queue.push(std::forward<_U>(_elem));
		}
		m_cv.notify_one();
	}

	_T pop()
	{
		std::unique_lock<std::mutex> lock{x_mutex};
		m_cv.wait(lock, [this]{ return !m_queue.empty(); });
		auto item = std::move(m_queue.front());
		m_queue.pop();
		return item;
	}

	std::pair<bool, _T> tryPop(int milliseconds) {
		std::unique_lock<std::mutex> lock{ x_mutex };
		auto ret = m_cv.wait_for(lock, std::chrono::milliseconds(milliseconds), [this]{ return !m_queue.empty(); });
		if(!ret)
		{
			return std::make_pair(ret, _T());
		}
		auto item = std::move(m_queue.front());
		m_queue.pop();
		return std::make_pair(ret, item);
	}

private:
	_QueueT m_queue;
	std::mutex x_mutex;
	std::condition_variable m_cv;
};

}
