#include <http/http>

#include <gtest/gtest.h>

const auto client = http::client();

TEST(HttpTest, ResponseStatus) {
    auto res = client.request("http://192.168.8.2:9200");

    ASSERT_EQ(200, res.status());
}
