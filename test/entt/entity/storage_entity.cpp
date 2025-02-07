#include <algorithm>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/storage.hpp>
#include "../common/config.h"

TEST(StorageEntity, Constructors) {
    entt::storage<entt::entity> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());

    pool = entt::storage<entt::entity>{std::allocator<entt::entity>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
}

TEST(StorageEntity, Move) {
    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{3});

    ASSERT_TRUE(std::is_move_constructible_v<decltype(pool)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(pool)>);

    entt::storage<entt::entity> other{std::move(pool)};

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(other.type(), entt::type_id<entt::entity>());

    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{3});

    entt::storage<entt::entity> extended{std::move(other), std::allocator<entt::entity>{}};

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(other.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(extended.type(), entt::type_id<entt::entity>());

    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(extended.at(0u), entt::entity{3});

    pool = std::move(extended);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(extended.empty());

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(other.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(extended.type(), entt::type_id<entt::entity>());

    ASSERT_EQ(pool.at(0u), entt::entity{3});
    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(extended.at(0u), static_cast<entt::entity>(entt::null));

    other = entt::storage<entt::entity>{};
    other.emplace(entt::entity{42});
    other = std::move(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(other.type(), entt::type_id<entt::entity>());

    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{3});
}

TEST(StorageEntity, Swap) {
    entt::storage<entt::entity> pool;
    entt::storage<entt::entity> other;

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(other.type(), entt::type_id<entt::entity>());

    pool.emplace(entt::entity{42});

    other.emplace(entt::entity{9});
    other.emplace(entt::entity{3});
    other.erase(entt::entity{9});

    ASSERT_EQ(pool.size(), 43u);
    ASSERT_EQ(other.size(), 10u);

    pool.swap(other);

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(other.type(), entt::type_id<entt::entity>());

    ASSERT_EQ(pool.size(), 10u);
    ASSERT_EQ(other.size(), 43u);

    ASSERT_EQ(pool.at(0u), entt::entity{3});
    ASSERT_EQ(other.at(0u), entt::entity{42});
}

TEST(StorageEntity, Getters) {
    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{41});

    testing::StaticAssertTypeEq<decltype(pool.get({})), void>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), void>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<>>();

    ASSERT_NO_FATAL_FAILURE(pool.get(entt::entity{41}));
    ASSERT_NO_FATAL_FAILURE(std::as_const(pool).get(entt::entity{41}));

    ASSERT_EQ(pool.get_as_tuple(entt::entity{41}), std::make_tuple());
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entt::entity{41}), std::make_tuple());
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, Getters) {
    entt::storage<entt::entity> pool;

    ASSERT_DEATH(pool.get(entt::entity{41}), "");
    ASSERT_DEATH(std::as_const(pool).get(entt::entity{41}), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entt::entity{41}), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entt::entity{41}), "");
}

TEST(StorageEntity, Emplace) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity> pool;
    entt::entity entity[2u]{};

    ASSERT_EQ(pool.emplace(), entt::entity{0});
    ASSERT_EQ(pool.emplace(entt::null), entt::entity{1});
    ASSERT_EQ(pool.emplace(entt::tombstone), entt::entity{2});
    ASSERT_EQ(pool.emplace(entt::entity{0}), entt::entity{3});
    ASSERT_EQ(pool.emplace(traits_type::construct(1, 1)), entt::entity{4});
    ASSERT_EQ(pool.emplace(traits_type::construct(6, 3)), traits_type::construct(6, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{1}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{2}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{4}), pool.in_use());
    ASSERT_GE(pool.index(entt::entity{5}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(6, 3)), pool.in_use());

    ASSERT_EQ(pool.emplace(traits_type::construct(5, 42)), traits_type::construct(5, 42));
    ASSERT_EQ(pool.emplace(traits_type::construct(5, 43)), entt::entity{7});

    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.emplace(), traits_type::construct(2, 1));

    pool.erase(traits_type::construct(2, 1));
    pool.insert(entity, entity + 2u);

    ASSERT_EQ(entity[0u], traits_type::construct(2, 2));
    ASSERT_EQ(entity[1u], entt::entity{8});
}

TEST(StorageEntity, TryEmplace) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity> pool;

    ASSERT_EQ(*pool.push(entt::null), entt::entity{0});
    ASSERT_EQ(*pool.push(entt::tombstone), entt::entity{1});
    ASSERT_EQ(*pool.push(entt::entity{0}), entt::entity{2});
    ASSERT_EQ(*pool.push(traits_type::construct(1, 1)), entt::entity{3});
    ASSERT_EQ(*pool.push(traits_type::construct(5, 3)), traits_type::construct(5, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{1}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{2}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_GE(pool.index(entt::entity{4}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(5, 3)), pool.in_use());

    ASSERT_EQ(*pool.push(traits_type::construct(4, 42)), traits_type::construct(4, 42));
    ASSERT_EQ(*pool.push(traits_type::construct(4, 43)), entt::entity{6});

    entt::entity entity[2u]{entt::entity{1}, traits_type::construct(5, 3)};

    pool.erase(entity, entity + 2u);
    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.current(entity[0u]), 1);
    ASSERT_EQ(pool.current(entity[1u]), 4);
    ASSERT_EQ(pool.current(entt::entity{2}), 1);

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(1, 1)), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(2, 1)), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(4, 42)), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(5, 4)), pool.in_use());

    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(2, 1));
    ASSERT_EQ(*pool.push(traits_type::construct(1, 3)), traits_type::construct(1, 3));
    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(5, 4));
    ASSERT_EQ(*pool.push(entt::null), entt::entity{7});
}

TEST(StorageEntity, Patch) {
    entt::storage<entt::entity> pool;
    const auto entity = pool.emplace();

    int counter = 0;
    auto callback = [&counter]() { ++counter; };

    ASSERT_EQ(counter, 0);

    pool.patch(entity);
    pool.patch(entity, callback);
    pool.patch(entity, callback, callback);

    ASSERT_EQ(counter, 3);
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, Patch) {
    entt::storage<entt::entity> pool;

    ASSERT_DEATH(pool.patch(entt::null), "");
}

TEST(StorageEntity, Insert) {
    entt::storage<entt::entity> pool;
    entt::entity entity[2u]{};

    pool.insert(std::begin(entity), std::end(entity));

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.erase(std::begin(entity), std::end(entity));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);

    pool.insert(entity, entity + 1u);

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
}

TEST(StorageEntity, Pack) {
    entt::storage<entt::entity> pool;
    entt::entity entity[3u]{entt::entity{1}, entt::entity{3}, entt::entity{42}};

    pool.push(entity, entity + 3u);
    std::swap(entity[0u], entity[1u]);

    const auto len = pool.pack(entity + 1u, entity + 3u);
    auto it = pool.each().cbegin().base();

    ASSERT_NE(it, pool.cbegin());
    ASSERT_NE(it, pool.cend());

    ASSERT_EQ(len, 2u);
    ASSERT_NE(it + len, pool.cend());
    ASSERT_EQ(it + len + 1u, pool.cend());

    ASSERT_EQ(*it++, entity[1u]);
    ASSERT_EQ(*it++, entity[2u]);

    ASSERT_NE(it, pool.cend());
    ASSERT_EQ(*it++, entity[0u]);
    ASSERT_EQ(it, pool.cend());
}

TEST(StorageEntity, InUse) {
    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{0});

    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(pool.free_list(), 1u);

    pool.in_use(0u);

    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(pool.free_list(), 0u);

    pool.in_use(1u);

    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(pool.free_list(), 1u);
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, InUse) {
    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{0});

    ASSERT_DEATH(pool.in_use(2u), "");
}

TEST(StorageEntity, Iterable) {
    using iterator = typename entt::storage<entt::entity>::iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

    pool.erase(entt::entity{3});

    auto iterable = pool.each();

    iterator end{iterable.begin()};
    iterator begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_NE(begin.base(), pool.begin());
    ASSERT_EQ(begin.base(), pool.end() - pool.in_use());
    ASSERT_EQ(end.base(), pool.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{42});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{42});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.end() - 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.end());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ConstIterable) {
    using iterator = typename entt::storage<entt::entity>::const_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

    pool.erase(entt::entity{3});

    auto iterable = std::as_const(pool).each();

    iterator end{iterable.cbegin()};
    iterator begin{};
    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_NE(begin.base(), pool.begin());
    ASSERT_EQ(begin.base(), pool.end() - pool.in_use());
    ASSERT_EQ(end.base(), pool.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{42});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{42});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.end() - 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.end());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, IterableIteratorConversion) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    typename entt::storage<entt::entity>::iterable::iterator it = pool.each().begin();
    typename entt::storage<entt::entity>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(StorageEntity, IterableAlgorithmCompatibility) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST(StorageEntity, ReverseIterable) {
    using iterator = typename entt::storage<entt::entity>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

    pool.erase(entt::entity{3});

    auto iterable = pool.reach();

    iterator end{iterable.begin()};
    iterator begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), pool.rbegin());
    ASSERT_EQ(end.base(), pool.rbegin() + pool.in_use());
    ASSERT_NE(end.base(), pool.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.rbegin() + 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.rbegin() + 2);

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ReverseConstIterable) {
    using iterator = typename entt::storage<entt::entity>::const_reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

    pool.erase(entt::entity{3});

    auto iterable = std::as_const(pool).reach();

    iterator end{iterable.cbegin()};
    iterator begin{};
    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), pool.rbegin());
    ASSERT_EQ(end.base(), pool.rbegin() + pool.in_use());
    ASSERT_NE(end.base(), pool.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.rbegin() + 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.rbegin() + 2);

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ReverseIterableIteratorConversion) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    typename entt::storage<entt::entity>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<entt::entity>::const_reverse_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(StorageEntity, ReverseIterableAlgorithmCompatibility) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}
