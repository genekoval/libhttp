#include <http/http>

#include <gtest/gtest.h>

using namespace std::literals;

TEST(URL, FromString) {
    const http::url url = "https://example.com/hello/world?a=foo&b=bar#main";

    EXPECT_EQ("https"sv, url.scheme());
    EXPECT_EQ("example.com"sv, url.host());
    EXPECT_EQ(443, url.port_or_default());
    EXPECT_EQ("/hello/world"sv, url.path());
    EXPECT_EQ("a=foo&b=bar"sv, url.query());
    EXPECT_EQ("main"sv, url.fragment());
}

TEST(URL, ToString) {
    auto url = http::url();

    url.scheme("https");
    url.host("example.com");
    url.port(443);
    url.path("/hello/world");
    url.query("a=foo&b=bar");
    url.fragment("main");

    const auto string = url.string();

    EXPECT_EQ("https://example.com:443/hello/world?a=foo&b=bar#main"sv, string);
}

TEST(URL, Redirect) {
    http::url url = "https://example.com/foo/bar?n=42";

    url = "../baz?v=true";

    EXPECT_EQ("https://example.com/baz?v=true"sv, url.string());
}

TEST(URL, FormatPath) {
    http::url url = "https://example.com";

    url.path("/foo/{}/bar", 42);

    EXPECT_EQ("https://example.com/foo/42/bar"sv, url.string());
}

TEST(URL, PathComponents) {
    http::url url = "https://example.com";

    url.path_components(42, "foo", false);

    EXPECT_EQ("https://example.com/42/foo/false"sv, url.string());
}

TEST(URL, Query) {
    http::url url = "https://example.com";

    url.query("q", "foo");
    url.query("t", 500);
    url.query("bar", true);

    EXPECT_EQ("https://example.com/?q=foo&t=500&bar=true"sv, url.string());
}

TEST(URL, FmtSupport) {
    constexpr auto str = "https://example.com/hello/world"sv;

    const http::url url = str;
    const auto string = fmt::to_string(url);

    EXPECT_EQ(str, string);
}

TEST(URL, JSONSupport) {
    constexpr auto str = "https://example.com/hello/world"sv;

    auto json = http::json();

    json["url"] = str;

    const auto url = json["url"].get<http::url>();

    EXPECT_EQ(str, url.string());

    json["encoded"] = url;

    EXPECT_EQ(str, json["encoded"].get<std::string>());
}
