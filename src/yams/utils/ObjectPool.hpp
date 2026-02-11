#pragma once

#include <concurrentqueue.h>
#include <functional>
#include <memory>
#include <source_location>
#include <type_traits>

#include <slog++/slog++.hpp>

namespace yams {

namespace details {
template <
    typename T,
    std::enable_if_t<std::is_default_constructible_v<T>> * = nullptr>
class DefaultNew {
public:
	T *operator()() const {
		return new T();
	}
};
} // namespace details

template <
    typename T,
    typename Constructor = details::DefaultNew<T>,
    typename Deleter     = std::default_delete<T>>

//
class ObjectPool
    : public std::enable_shared_from_this<ObjectPool<T, Constructor, Deleter>> {

public:
	using Ptr = std::shared_ptr<ObjectPool<T, Constructor, Deleter>>;

	using ObjectPtr = std::unique_ptr<T, std::function<void(T *)>>;

	static Ptr Create(
	    Constructor &&constructor = Constructor{}, Deleter &&deleter = Deleter{}
	) {
		return Ptr{new ObjectPool<T, Constructor, Deleter>{
		    std::move(constructor),
		    std::move(deleter)
		}};
	}

	~ObjectPool() {
		slog::DDebug("destructor called", slog::Location());
		T *obj{nullptr};
		while (d_queue.try_dequeue(obj) == true) {
			d_deleter(obj);
		}
	}

	ObjectPool(const ObjectPool &)            = delete;
	ObjectPool &operator=(const ObjectPool &) = delete;
	ObjectPool(ObjectPool &&)                 = delete;
	ObjectPool &operator=(ObjectPool &&)      = delete;

	template <typename... Function>
	inline ObjectPtr Get(Function &&...onDelete) {
		T *obj{nullptr};

		auto deleter = [self = this->shared_from_this(), onDelete...](T *obj) {
			(onDelete(obj), ...);
			self->d_queue.enqueue(obj);
		};

		if (d_queue.try_dequeue(obj) == true) {
			return {obj, deleter};
		}
		d_allocated.fetch_add(1);
		return {d_constructor(), deleter};
	}

	inline size_t PoolSize() const {
		return d_allocated.load();
	}

private:
	ObjectPool(Constructor &&constructor, Deleter &&deleter)
	    : d_constructor{std::move(constructor)}
	    , d_deleter{std::move(deleter)} {}

	Constructor                      d_constructor;
	Deleter                          d_deleter;
	moodycamel::ConcurrentQueue<T *> d_queue;
	std::atomic<size_t>              d_allocated;
};
} // namespace yams
