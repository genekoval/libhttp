#include <http/http>

#include <gtest/gtest.h>

constexpr auto server = "http://127.0.0.1:8080";

class HttpTest : public testing::Test {
protected:
    http::client client;
    http::request request;

    HttpTest() {
        request.url().set(CURLUPART_URL, server);
    }
};

TEST_F(HttpTest, BlockingRequest) {
    request.url().path("/");

    auto task = request.collect();
    const auto res = request.perform();
    const auto body = std::move(task).result();

    EXPECT_TRUE(res.ok());
    EXPECT_EQ("A Test Server!", body);
}

TEST_F(HttpTest, NonblockingRequest) {
    netcore::run([this]() -> ext::task<> {
        request.url().path("/");

        auto task = request.collect();
        const auto res = co_await request.perform(client);
        const auto body = std::move(task).result();

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("A Test Server!", body);
    }());
}

TEST_F(HttpTest, Send) {
    netcore::run([this]() -> ext::task<> {
        request.url().path("/echo");
        request.method(http::method::POST);
        request.body("Hello, world!");

        auto task = request.collect();
        const auto res = co_await request.perform(client);
        const auto body = std::move(task).result();

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("Your request (POST): Hello, world!", body);
    }());
}

TEST_F(HttpTest, GetBodyData) {
    netcore::run([this]() -> ext::task<> {
        request.url().path("/echo");
        const auto method = request.method(http::method::POST, "GET");
        request.body("Body Data");

        auto task = request.collect();
        const auto res = co_await request.perform(client);
        const auto body = std::move(task).result();

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("Your request (GET): Body Data", body);
    }());
}
