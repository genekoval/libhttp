#include <http/http>

#include <gtest/gtest.h>

const auto client = http::client();

namespace {
    namespace internal {
        struct book {
            std::string title;
            std::string author;
            int year;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
            book,
            title,
            author,
            year
        )
    }
}

TEST(HttpTest, ReadText) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/");

    auto res = req.perform();

    ASSERT_TRUE(res.ok());
    ASSERT_EQ("A Test Server!", res.text());
}

TEST(HttpTest, ReadJson) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/json");

    auto res = req.perform();

    ASSERT_TRUE(res.ok());

    const auto book = res.json().get<internal::book>();

    ASSERT_EQ("Hello Golang", book.title);
    ASSERT_EQ("John Mike", book.author);
    ASSERT_EQ(2021, book.year);
}

TEST(HttpTest, Send) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/echo");
    req.method(http::method::POST);
    req.body("Hello, world!");

    auto res = req.perform();

    ASSERT_TRUE(res.ok());

    const auto text = res.text();

    ASSERT_EQ("Your request (POST): Hello, world!", text);
}

TEST(HttpTest, GetBodyData) {
    auto req = http::request();
    req.url("http://127.0.0.1:8080/echo");
    req.method(http::method::POST);
    req.method("GET");
    req.body("Body Data");

    auto res = req.perform();

    ASSERT_TRUE(res.ok());

    const auto text = res.text();

    ASSERT_EQ("Your request (GET): Body Data", text);
}
