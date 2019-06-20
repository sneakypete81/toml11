#define BOOST_TEST_MODULE "test_find"

#ifdef UNITTEST_FRAMEWORK_LIBRARY_EXIST
#include <boost/test/unit_test.hpp>
#else
#define BOOST_TEST_NO_LIB
#include <boost/test/included/unit_test.hpp>
#endif

#include <toml.hpp>
#include <map>
#include <unordered_map>
#include <list>
#include <deque>
#include <array>
#if __cplusplus >= 201703L
#include <string_view>
#endif
#include <tuple>

using test_value_types = std::tuple<
    toml::value,
    toml::basic_value<toml::preserve_comments>,
    toml::basic_value<toml::discard_comments,  std::map, std::deque>,
    toml::basic_value<toml::preserve_comments, std::map, std::deque>
    >;

BOOST_AUTO_TEST_CASE(test_find_throws)
{
    {
        // value is not a table
        toml::value v(true);
        BOOST_CHECK_THROW(toml::find<toml::boolean>(v, "key"), toml::type_error);
    }
    {
        // the value corresponding to the key is not the expected type
        toml::value v{{"key", 42}};
        BOOST_CHECK_THROW(toml::find<toml::boolean>(v, "key"), toml::type_error);
    }
    {
        // the value corresponding to the key is not found
        toml::value v{{"key", 42}};
        BOOST_CHECK_THROW(toml::find<toml::integer>(v, "different_key"),
                          std::out_of_range);
    }
    {
        // the positive control.
        toml::value v{{"key", 42}};
        BOOST_CHECK_EQUAL(42, toml::find<int>(v, "key"));
    }
}

BOOST_AUTO_TEST_CASE(test_find_recursive)
{
    // recursively search tables
    {
        toml::value v{
            {"a", {
                {"b", {
                    {"c", {
                        {"d", 42}
                    }}
                }}
            }}
        };
        BOOST_CHECK_EQUAL(42, toml::find<int>(v, "a", "b", "c", "d"));

        // reference that can be used to modify the content
        auto& num = toml::find<toml::integer>(v, "a", "b", "c", "d");
        num = 54;
        BOOST_CHECK_EQUAL(54, toml::find<int>(v, "a", "b", "c", "d"));

        const std::string a("a"), b("b"), c("c"), d("d");
        auto& num2 = toml::find<toml::integer>(v, a, b, c, d);
        num2 = 42;
        BOOST_CHECK_EQUAL(42, toml::find<int>(v, a, b, c, d));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_exact, value_type, test_value_types)
{
    {
        value_type v{{"key", true}};
        BOOST_CHECK_EQUAL(true, toml::find<toml::boolean>(v, "key"));

        toml::find<toml::boolean>(v, "key") = false;
        BOOST_CHECK_EQUAL(false, toml::find<toml::boolean>(v, "key"));
    }
    {
        value_type v{{"key", 42}};
        BOOST_CHECK_EQUAL(toml::integer(42), toml::find<toml::integer>(v, "key"));

        toml::find<toml::integer>(v, "key") = 54;
        BOOST_CHECK_EQUAL(toml::integer(54), toml::find<toml::integer>(v, "key"));
    }
    {
        value_type v{{"key", 3.14}};
        BOOST_CHECK_EQUAL(toml::floating(3.14), toml::find<toml::floating>(v, "key"));

        toml::find<toml::floating>(v, "key") = 2.71;
        BOOST_CHECK_EQUAL(toml::floating(2.71), toml::find<toml::floating>(v, "key"));
    }
    {
        value_type v{{"key", "foo"}};
        BOOST_CHECK_EQUAL(toml::string("foo", toml::string_t::basic),
                          toml::find<toml::string>(v, "key"));

        toml::find<toml::string>(v, "key").str += "bar";
        BOOST_CHECK_EQUAL(toml::string("foobar", toml::string_t::basic),
                          toml::find<toml::string>(v, "key"));
    }
    {
        value_type v{{"key", value_type("foo", toml::string_t::literal)}};
        BOOST_CHECK_EQUAL(toml::string("foo", toml::string_t::literal),
                          toml::find<toml::string>(v, "key"));

        toml::find<toml::string>(v, "key").str += "bar";
        BOOST_CHECK_EQUAL(toml::string("foobar", toml::string_t::literal),
                          toml::find<toml::string>(v, "key"));
    }
    {
        toml::local_date d(2018, toml::month_t::Apr, 22);
        value_type v{{"key", d}};
        BOOST_CHECK(d == toml::find<toml::local_date>(v, "key"));

        toml::find<toml::local_date>(v, "key").year = 2017;
        d.year = 2017;
        BOOST_CHECK(d == toml::find<toml::local_date>(v, "key"));
    }
    {
        toml::local_time t(12, 30, 45);
        value_type v{{"key", t}};
        BOOST_CHECK(t == toml::find<toml::local_time>(v, "key"));

        toml::find<toml::local_time>(v, "key").hour = 9;
        t.hour = 9;
        BOOST_CHECK(t == toml::find<toml::local_time>(v, "key"));
    }
    {
        toml::local_datetime dt(toml::local_date(2018, toml::month_t::Apr, 22),
                                toml::local_time(12, 30, 45));
        value_type v{{"key", dt}};
        BOOST_CHECK(dt == toml::find<toml::local_datetime>(v, "key"));

        toml::find<toml::local_datetime>(v, "key").date.year = 2017;
        dt.date.year = 2017;
        BOOST_CHECK(dt == toml::find<toml::local_datetime>(v, "key"));
    }
    {
        toml::offset_datetime dt(toml::local_datetime(
                    toml::local_date(2018, toml::month_t::Apr, 22),
                    toml::local_time(12, 30, 45)), toml::time_offset(9, 0));
        value_type v{{"key", dt}};
        BOOST_CHECK(dt == toml::find<toml::offset_datetime>(v, "key"));

        toml::find<toml::offset_datetime>(v, "key").date.year = 2017;
        dt.date.year = 2017;
        BOOST_CHECK(dt == toml::find<toml::offset_datetime>(v, "key"));
    }
    {
        typename value_type::array_type vec;
        vec.push_back(value_type(42));
        vec.push_back(value_type(54));
        value_type v{{"key", vec}};

        const bool result1 = (vec == toml::find<typename value_type::array_type>(v, "key"));
        BOOST_CHECK(result1);

        toml::find<typename value_type::array_type>(v, "key").push_back(value_type(123));
        vec.push_back(value_type(123));

        const bool result2 = (vec == toml::find<typename value_type::array_type>(v, "key"));
        BOOST_CHECK(result2);
    }
    {
        typename value_type::table_type tab;
        tab["key1"] = value_type(42);
        tab["key2"] = value_type(3.14);
        value_type v{{"key", tab}};
        const bool result1 = (tab == toml::find<typename value_type::table_type>(v, "key"));
        BOOST_CHECK(result1);

        toml::find<typename value_type::table_type>(v, "key")["key3"] = value_type(123);
        tab["key3"] = value_type(123);
        const bool result2 = (tab == toml::find<typename value_type::table_type>(v, "key"));
        BOOST_CHECK(result2);
    }
    {
        value_type v1(42);
        value_type v{{"key", v1}};
        BOOST_CHECK(v1 == toml::find(v, "key"));

        value_type v2(54);
        toml::find(v, "key") = v2;
        BOOST_CHECK(v2 == toml::find(v, "key"));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_integer_type, value_type, test_value_types)
{
    {
        value_type v{{"key", 42}};
        BOOST_CHECK_EQUAL(int(42),           toml::find<int          >(v, "key"));
        BOOST_CHECK_EQUAL(short(42),         toml::find<short        >(v, "key"));
        BOOST_CHECK_EQUAL(char(42),          toml::find<char         >(v, "key"));
        BOOST_CHECK_EQUAL(unsigned(42),      toml::find<unsigned     >(v, "key"));
        BOOST_CHECK_EQUAL(long(42),          toml::find<long         >(v, "key"));
        BOOST_CHECK_EQUAL(std::int64_t(42),  toml::find<std::int64_t >(v, "key"));
        BOOST_CHECK_EQUAL(std::uint64_t(42), toml::find<std::uint64_t>(v, "key"));
        BOOST_CHECK_EQUAL(std::int16_t(42),  toml::find<std::int16_t >(v, "key"));
        BOOST_CHECK_EQUAL(std::uint16_t(42), toml::find<std::uint16_t>(v, "key"));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_floating_type, value_type, test_value_types)
{
    {
        value_type v{{"key", 3.14}};
        BOOST_CHECK_EQUAL(static_cast<float      >(3.14), toml::find<float      >(v, "key"));
        BOOST_CHECK_EQUAL(static_cast<double     >(3.14), toml::find<double     >(v, "key"));
        BOOST_CHECK_EQUAL(static_cast<long double>(3.14), toml::find<long double>(v, "key"));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_string_type, value_type, test_value_types)
{
    {
        value_type v{{"key", toml::string("foo", toml::string_t::basic)}};
        BOOST_CHECK_EQUAL("foo", toml::find<std::string>(v, "key"));
        toml::find<std::string>(v, "key") += "bar";
        BOOST_CHECK_EQUAL("foobar", toml::find<std::string>(v, "key"));
    }
    {
        value_type v{{"key", toml::string("foo", toml::string_t::literal)}};
        BOOST_CHECK_EQUAL("foo", toml::find<std::string>(v, "key"));
        toml::find<std::string>(v, "key") += "bar";
        BOOST_CHECK_EQUAL("foobar", toml::find<std::string>(v, "key"));
    }

#if __cplusplus >= 201703L
    {
        value_type v{{"key", toml::string("foo", toml::string_t::basic)}};
        BOOST_CHECK_EQUAL("foo", toml::find<std::string_view>(v, "key"));
    }
    {
        value_type v{{"key", toml::string("foo", toml::string_t::literal)}};
        BOOST_CHECK_EQUAL("foo", toml::find<std::string_view>(v, "key"));
    }
#endif
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_toml_array, value_type, test_value_types)
{
    value_type v{{"key", {42, 54, 69, 72}}};

    const std::vector<int>         vec = toml::find<std::vector<int>>(v, "key");
    const std::list<short>         lst = toml::find<std::list<short>>(v, "key");
    const std::deque<std::int64_t> deq = toml::find<std::deque<std::int64_t>>(v, "key");

    BOOST_CHECK_EQUAL(42, vec.at(0));
    BOOST_CHECK_EQUAL(54, vec.at(1));
    BOOST_CHECK_EQUAL(69, vec.at(2));
    BOOST_CHECK_EQUAL(72, vec.at(3));

    std::list<short>::const_iterator iter = lst.begin();
    BOOST_CHECK_EQUAL(static_cast<short>(42), *(iter++));
    BOOST_CHECK_EQUAL(static_cast<short>(54), *(iter++));
    BOOST_CHECK_EQUAL(static_cast<short>(69), *(iter++));
    BOOST_CHECK_EQUAL(static_cast<short>(72), *(iter++));

    BOOST_CHECK_EQUAL(static_cast<std::int64_t>(42), deq.at(0));
    BOOST_CHECK_EQUAL(static_cast<std::int64_t>(54), deq.at(1));
    BOOST_CHECK_EQUAL(static_cast<std::int64_t>(69), deq.at(2));
    BOOST_CHECK_EQUAL(static_cast<std::int64_t>(72), deq.at(3));

    std::array<int, 4> ary = toml::find<std::array<int, 4>>(v, "key");
    BOOST_CHECK_EQUAL(static_cast<int>(42), ary.at(0));
    BOOST_CHECK_EQUAL(static_cast<int>(54), ary.at(1));
    BOOST_CHECK_EQUAL(static_cast<int>(69), ary.at(2));
    BOOST_CHECK_EQUAL(static_cast<int>(72), ary.at(3));

    std::tuple<int, short, unsigned, long> tpl =
        toml::find<std::tuple<int, short, unsigned, long>>(v, "key");
    BOOST_CHECK_EQUAL(static_cast<int     >(42), std::get<0>(tpl));
    BOOST_CHECK_EQUAL(static_cast<short   >(54), std::get<1>(tpl));
    BOOST_CHECK_EQUAL(static_cast<unsigned>(69), std::get<2>(tpl));
    BOOST_CHECK_EQUAL(static_cast<long    >(72), std::get<3>(tpl));

    value_type p{{"key", {3.14, 2.71}}};
    std::pair<double, double> pr = toml::find<std::pair<double, double> >(p, "key");
    BOOST_CHECK_EQUAL(3.14, pr.first);
    BOOST_CHECK_EQUAL(2.71, pr.second);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_toml_array_of_array, value_type, test_value_types)
{
    value_type v1{42, 54, 69, 72};
    value_type v2{"foo", "bar", "baz"};
    value_type v{{"key", {v1, v2}}};

    std::pair<std::vector<int>, std::vector<std::string>> p =
        toml::find<std::pair<std::vector<int>, std::vector<std::string>>>(v, "key");

    BOOST_CHECK_EQUAL(p.first.at(0), 42);
    BOOST_CHECK_EQUAL(p.first.at(1), 54);
    BOOST_CHECK_EQUAL(p.first.at(2), 69);
    BOOST_CHECK_EQUAL(p.first.at(3), 72);

    BOOST_CHECK_EQUAL(p.second.at(0), "foo");
    BOOST_CHECK_EQUAL(p.second.at(1), "bar");
    BOOST_CHECK_EQUAL(p.second.at(2), "baz");

    std::tuple<std::vector<int>, std::vector<std::string>> t =
        toml::find<std::tuple<std::vector<int>, std::vector<std::string>>>(v, "key");

    BOOST_CHECK_EQUAL(std::get<0>(t).at(0), 42);
    BOOST_CHECK_EQUAL(std::get<0>(t).at(1), 54);
    BOOST_CHECK_EQUAL(std::get<0>(t).at(2), 69);
    BOOST_CHECK_EQUAL(std::get<0>(t).at(3), 72);

    BOOST_CHECK_EQUAL(std::get<1>(t).at(0), "foo");
    BOOST_CHECK_EQUAL(std::get<1>(t).at(1), "bar");
    BOOST_CHECK_EQUAL(std::get<1>(t).at(2), "baz");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_toml_table, value_type, test_value_types)
{
    value_type v1{{"key", {
            {"key1", 1}, {"key2", 2}, {"key3", 3}, {"key4", 4}
        }}};
    const auto v = toml::find<std::map<std::string, int>>(v1, "key");
    BOOST_CHECK_EQUAL(v.at("key1"), 1);
    BOOST_CHECK_EQUAL(v.at("key2"), 2);
    BOOST_CHECK_EQUAL(v.at("key3"), 3);
    BOOST_CHECK_EQUAL(v.at("key4"), 4);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_toml_local_date, value_type, test_value_types)
{
    value_type v1{{"key", toml::local_date{2018, toml::month_t::Apr, 1}}};
    const auto date = std::chrono::system_clock::to_time_t(
            toml::find<std::chrono::system_clock::time_point>(v1, "key"));

    std::tm t;
    t.tm_year = 2018 - 1900;
    t.tm_mon  = 4    - 1;
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min  = 0;
    t.tm_sec  = 0;
    t.tm_isdst = -1;
    const auto c = std::mktime(&t);
    BOOST_CHECK_EQUAL(c, date);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_toml_local_time, value_type, test_value_types)
{
    value_type v1{{"key", toml::local_time{12, 30, 45}}};
    const auto time = toml::find<std::chrono::seconds>(v1, "key");
    BOOST_CHECK(time == std::chrono::hours(12) +
                        std::chrono::minutes(30) + std::chrono::seconds(45));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_find_toml_local_datetime, value_type, test_value_types)
{
    value_type v1{{"key", toml::local_datetime(
                toml::local_date{2018, toml::month_t::Apr, 1},
                toml::local_time{12, 30, 45})}};

    const auto date = std::chrono::system_clock::to_time_t(
            toml::find<std::chrono::system_clock::time_point>(v1, "key"));
    std::tm t;
    t.tm_year = 2018 - 1900;
    t.tm_mon  = 4    - 1;
    t.tm_mday = 1;
    t.tm_hour = 12;
    t.tm_min  = 30;
    t.tm_sec  = 45;
    t.tm_isdst = -1;
    const auto c = std::mktime(&t);
    BOOST_CHECK_EQUAL(c, date);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_get_toml_offset_datetime, value_type, test_value_types)
{
    {
        value_type v1{{"key", toml::offset_datetime(
                    toml::local_date{2018, toml::month_t::Apr, 1},
                    toml::local_time{12, 30, 0},
                    toml::time_offset{9, 0})}};
        //    2018-04-01T12:30:00+09:00
        // == 2018-04-01T03:30:00Z

        const auto date = toml::find<std::chrono::system_clock::time_point>(v1, "key");
        const auto timet = std::chrono::system_clock::to_time_t(date);

        // get time_t as gmtime (2018-04-01T03:30:00Z)
        const auto tmp = std::gmtime(std::addressof(timet)); // XXX not threadsafe!
        BOOST_CHECK(tmp);
        const auto tm = *tmp;
        BOOST_CHECK_EQUAL(tm.tm_year + 1900, 2018);
        BOOST_CHECK_EQUAL(tm.tm_mon  + 1,       4);
        BOOST_CHECK_EQUAL(tm.tm_mday,           1);
        BOOST_CHECK_EQUAL(tm.tm_hour,           3);
        BOOST_CHECK_EQUAL(tm.tm_min,           30);
        BOOST_CHECK_EQUAL(tm.tm_sec,            0);
    }

    {
        value_type v1{{"key", toml::offset_datetime(
                    toml::local_date{2018, toml::month_t::Apr, 1},
                    toml::local_time{12, 30, 0},
                    toml::time_offset{-8, 0})}};
        //    2018-04-01T12:30:00-08:00
        // == 2018-04-01T20:30:00Z

        const auto date = toml::find<std::chrono::system_clock::time_point>(v1, "key");
        const auto timet = std::chrono::system_clock::to_time_t(date);

        // get time_t as gmtime (2018-04-01T03:30:00Z)
        const auto tmp = std::gmtime(std::addressof(timet)); // XXX not threadsafe!
        BOOST_CHECK(tmp);
        const auto tm = *tmp;
        BOOST_CHECK_EQUAL(tm.tm_year + 1900, 2018);
        BOOST_CHECK_EQUAL(tm.tm_mon  + 1,       4);
        BOOST_CHECK_EQUAL(tm.tm_mday,           1);
        BOOST_CHECK_EQUAL(tm.tm_hour,          20);
        BOOST_CHECK_EQUAL(tm.tm_min,           30);
        BOOST_CHECK_EQUAL(tm.tm_sec,            0);
    }
}

