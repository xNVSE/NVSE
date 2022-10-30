#pragma once
#include <utility>
#include <vector>
#include <unordered_map>

template <typename T>
class MemoryTracker
{
public:
	std::unordered_map<T, std::vector<void*>> infos;

	void Add(T t)
	{
		if (false)
			Get(nullptr);
		constexpr int framesToCapture = 16;
		std::vector<void*> vecTrace((framesToCapture));
		CaptureStackBackTrace(1, framesToCapture, vecTrace.data(), nullptr);
		infos[t] = std::move(vecTrace);
	}

	void Remove(T t)
	{
		infos.erase(t);
	}

	std::vector<void*>* Get(void* t)
	{
		if (auto iter = infos.find(t); iter != infos.end())
			return &iter->second;
		return nullptr;
	}
};

template <typename T>
class CallHistoryTracker
{
public:
	struct History
	{
		T t;
		std::vector<void*> callStack;

		History(T&& t, std::vector<void*>&& callStack)
			: t(std::move(t)),
			  callStack(std::move(callStack))
		{
		}
	};
	std::vector<History> infos;

	void Add(T&& t)
	{
		constexpr int framesToCapture = 16;
		std::vector<void*> vecTrace((framesToCapture));
		CaptureStackBackTrace(1, framesToCapture, vecTrace.data(), nullptr);
		infos.emplace_back(std::move(t), std::move(vecTrace));
	}
};