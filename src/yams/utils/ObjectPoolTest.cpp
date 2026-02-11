#include <gtest/gtest.h>

#include "ObjectPool.hpp"

namespace yams {

class ObjectPoolTest : public ::testing::Test {
protected:
	using Pool =
	    ObjectPool<int, std::function<int *()>, std::function<void(int *)>>;

	Pool::Ptr pool;

	std::atomic<int> d_constructed, d_deleted;

	void SetUp() {
		pool = Pool::Create(
		    [&]() -> int * {
			    d_constructed.fetch_add(1);
			    return new int{0};
		    },
		    [&](int *obj) {
			    d_deleted.fetch_add(1);
			    delete obj;
		    }
		);
	}

	void TearDown() {
		// checks that on deletion we call destructors.
		pool.reset();
		EXPECT_EQ(d_constructed.load(), d_deleted.load());
	}
};

TEST_F(ObjectPoolTest, DefaultConstructive) {
	using PoolDefault = ObjectPool<int>;
	auto poolB        = PoolDefault::Create();
}

TEST_F(ObjectPoolTest, ReusesAllocated) {
	auto a = pool->Get();
	auto b = pool->Get();
	a.reset();
	auto c = pool->Get();
	EXPECT_EQ(d_constructed.load(), 2);
	b.reset();
	c.reset();
}

TEST_F(ObjectPoolTest, CanHookOnRelease) {
	std::atomic<bool> signaled{false};
	auto a = pool->Get([&signaled](int *v) { signaled.store(true); });
	a.reset();
	EXPECT_EQ(d_deleted.load(), 0);
	EXPECT_EQ(signaled.load(), true);
}

} // namespace yams
