#include <http/http>

#include <gtest/gtest.h>

const auto client = http::client();

TEST(HttpTest, ResponseStatus) {
    auto res = client.request("http://192.168.8.2:9200");

    const auto data = res.json();

    ASSERT_TRUE(res.ok());
}
