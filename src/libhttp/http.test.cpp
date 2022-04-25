#include <http/http>

#include <gtest/gtest.h>

const auto client = http::client();
auto mem = http::memory();

TEST(HttpTest, ReadText) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/");

    auto res = req.perform(mem);

    ASSERT_TRUE(res.ok());
    ASSERT_EQ("A Test Server!", mem.storage);
}

TEST(HttpTest, Send) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/echo");
    req.method(http::method::POST);
    req.body("Hello, world!");

    auto res = req.perform(mem);

    ASSERT_TRUE(res.ok());

    ASSERT_EQ("Your request (POST): Hello, world!", mem.storage);
}

TEST(HttpTest, GetBodyData) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/echo");
    req.method(http::method::POST);
    req.method("GET");
    req.body("Body Data");

    auto res = req.perform(mem);

    ASSERT_TRUE(res.ok());

    ASSERT_EQ("Your request (GET): Body Data", mem.storage);
}
