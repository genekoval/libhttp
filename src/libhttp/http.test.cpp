#include <http/http>

#include <gtest/gtest.h>

constexpr auto server = "http://127.0.0.1:8080";

const auto init = http::init();

auto req = http::request();

class HttpTest : public testing::Test {
protected:
    static auto SetUpTestSuite() -> void {
        req.url().set(CURLUPART_URL, server);
    }
};

TEST_F(HttpTest, ReadText) {
    netcore::run([]() -> ext::task<> {
        auto client = http::client();

        req.url().path("/");

        auto res = co_await req.perform(client);

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("A Test Server!", res.body());
    }());
}

TEST_F(HttpTest, Send) {
    netcore::run([]() -> ext::task<> {
        auto client = http::client();

        req.url().path("/echo");
        req.method(http::method::POST);
        req.body("Hello, world!");

        auto res = co_await req.perform(client);

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("Your request (POST): Hello, world!", res.body());
    }());
}

TEST_F(HttpTest, GetBodyData) {
    netcore::run([]() -> ext::task<> {
        auto client = http::client();

        req.url().path("/echo");
        const auto method = req.method(http::method::POST, "GET");
        req.body("Body Data");

        auto res = co_await req.perform(client);

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("Your request (GET): Body Data", res.body());
    }());
}
