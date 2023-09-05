#include <http/http>

#include <gtest/gtest.h>

constexpr auto server = "http://127.0.0.1:8080";

class HttpTest : public testing::Test {
protected:
    http::session session;
    http::request request;

    HttpTest() {
        request.url = server;
    }
};

TEST_F(HttpTest, BlockingRequest) {
    request.url.path("/");

    const auto res = request.perform();

    EXPECT_TRUE(res.ok());
    EXPECT_EQ("A Test Server!", res.data());
}

TEST_F(HttpTest, NonblockingRequest) {
    netcore::run([this]() -> ext::task<> {
        request.url.path("/");

        const auto res = co_await request.perform(session);

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("A Test Server!", res.data());
    }());
}

TEST_F(HttpTest, ResponseContentLength) {
    request.url.path("/");

    long content_length_during_transfer = 0;

    const auto read_data = [&]() -> ext::jtask<std::string> {
        auto stream = request.pipe();
        auto chunk = co_await stream.read();
        auto result = std::string();

        content_length_during_transfer = stream.expected_size();

        while (!chunk.empty()) {
            result.append(
                reinterpret_cast<const char*>(chunk.data()),
                chunk.size()
            );

            chunk = co_await stream.read();
        }

        co_return result;
    };

    auto task = read_data();
    const auto res = request.perform();
    const auto data = std::move(task).result();
    const auto content_length = res.content_length();

    EXPECT_TRUE(res.ok());
    EXPECT_EQ(content_length_during_transfer, content_length);
    EXPECT_EQ(content_length, data.size());
}

TEST_F(HttpTest, Send) {
    netcore::run([this]() -> ext::task<> {
        request.url.path("/echo");
        request.method = "POST";
        request.data("Hello, world!");

        const auto res = co_await request.perform(session);

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("Your request (POST): Hello, world!", res.data());
    }());
}

TEST_F(HttpTest, GetBodyData) {
    netcore::run([this]() -> ext::task<> {
        request.url.path("/echo");
        request.method = "GET";
        request.data("Body Data");

        const auto res = co_await request.perform(session);

        EXPECT_TRUE(res.ok());
        EXPECT_EQ("Your request (GET): Body Data", res.data());
    }());
}
