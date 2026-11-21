// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "data/data_media_types.h"
#include "history/history_item.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

class TimedCountDownLatch
{
public:
	explicit TimedCountDownLatch(int count)
		: count_(count) {
	}

	TimedCountDownLatch(const TimedCountDownLatch &) = delete;
	TimedCountDownLatch &operator=(const TimedCountDownLatch &) = delete;

	void countDown() {
		std::unique_lock lock(mutex_);
		if (count_ > 0) {
			count_--;
		}
		if (count_ == 0) {
			cv_.notify_all();
		}
	}

	bool await(std::chrono::milliseconds timeout) {
		std::unique_lock lock(mutex_);
		if (count_ == 0) {
			return true;
		}
		return cv_.wait_for(lock, timeout, [this] { return count_ == 0; });
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;
	int count_;
};

[[nodiscard]] TextWithTags extractText(not_null<HistoryItem*> item);
[[nodiscard]] bool mediaDownloadable(const Data::Media *media);

void readHistory(not_null<HistoryItem*> message);
void markReadAfterAction(not_null<History*> history);
