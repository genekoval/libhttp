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
    req.url().path("/");

    auto data = std::string();
    auto res = req.perform(data);

    ASSERT_TRUE(res.ok());
    ASSERT_EQ("A Test Server!", data);
}

TEST_F(HttpTest, Send) {
    req.url().path("/echo");
    req.method(http::method::POST);
    req.body("Hello, world!");

    auto data = std::string();
    auto res = req.perform(data);

    ASSERT_TRUE(res.ok());

    ASSERT_EQ("Your request (POST): Hello, world!", data);
}

TEST_F(HttpTest, GetBodyData) {
    req.url().path("/echo");
    const auto method = req.method(http::method::POST, "GET");
    req.body("Body Data");

    auto data = std::string();
    auto res = req.perform(data);

    ASSERT_TRUE(res.ok());

    ASSERT_EQ("Your request (GET): Body Data", data);
}
