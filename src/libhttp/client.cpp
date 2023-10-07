#include <http/client.hpp>

#include <fstream>

namespace fs = std::filesystem;

namespace http {
    client::client(std::string_view base_url) :
        base_url(base_url),
        session(nullptr) {}

    client::client(std::string_view base_url, http::session& session) :
        base_url(base_url),
        session(&session) {}

    auto client::del() const -> request { return method("DELETE"); }

    auto client::get() const -> request { return method("GET"); }

    auto client::head() const -> request { return method("HEAD"); }

    auto client::method(std::string_view method) const -> request {
        return request(method, base_url, session);
    }

    auto client::post() const -> request { return method("POST"); }

    auto client::put() const -> request { return method("PUT"); }

    client::request::request(
        std::string_view method,
        const url& base_url,
        http::session* session
    ) :
        session(session) {
        req.method = method;
        req.url = base_url;
    }

    auto client::request::data() -> std::string {
        auto res = req.perform();

        if (res.ok()) return std::move(res).data();
        throw error_code(res.status(), res.data());
    }

    auto client::request::data(
        std::string&& data,
        std::optional<std::reference_wrapper<const media_type>> content_type
    ) -> request& {
        req.data(std::move(data));
        if (content_type) req.content_type(content_type->get());
        return *this;
    }

    auto client::request::data_view(
        std::string_view data,
        std::optional<std::reference_wrapper<const media_type>> content_type
    ) -> request& {
        req.data_view(data);
        if (content_type) req.content_type(content_type->get());
        return *this;
    }

    auto client::request::data_task() -> ext::task<std::string> {
        auto res = co_await req.perform(*session);

        if (res.ok()) co_return std::move(res).data();
        throw error_code(res.status(), res.data());
    }

    auto client::request::download(const fs::path& location) -> void {
        req.download(location);

        const auto res = req.perform();

        if (res.ok()) return;
        throw error_code(res.status(), read_error_file(location));
    }

    auto client::request::download_task(const std::filesystem::path& location)
        -> ext::task<> {
        req.download(location);

        const auto res = co_await req.perform(*session);

        if (res.ok()) co_return;
        throw error_code(res.status(), read_error_file(location));
    }

    auto client::request::pipe(FILE* file) -> void {
        req.pipe(file);

        const auto res = req.perform();

        if (res.ok()) return;
        throw error_code(res.status(), "Response piped to stream");
    }

    auto client::request::pipe_task(FILE* file) -> ext::task<> {
        req.pipe(file);

        const auto res = co_await req.perform(*session);

        if (res.ok()) co_return;
        throw error_code(res.status(), "Response piped to stream");
    }

    auto client::request::read_error_file(const std::filesystem::path& path)
        -> std::string {
        auto contents = std::string();
        auto stream = std::ifstream(path, std::ios::in | std::ios::binary);

        if (stream) {
            stream.seekg(0, std::ios::end);
            contents.resize(stream.tellg());
            stream.seekg(0, std::ios::beg);

            stream.read(contents.data(), contents.size());

            stream.close();
        }

        fs::remove(path);

        return contents;
    }

    auto client::request::send() -> void {
        const auto res = req.perform();
        if (!res.ok()) throw error_code(res.status(), res.data());
    }

    auto client::request::send_task() -> ext::task<> {
        const auto res = co_await req.perform(*session);
        if (!res.ok()) throw error_code(res.status(), res.data());
    }

    auto client::request::text(
        std::string&& data,
        const media_type& content_type
    ) -> request& {
        req.data(std::move(data));
        req.content_type(content_type);
        return *this;
    }

    auto client::request::text_view(
        std::string_view data,
        const media_type& content_type
    ) -> request& {
        req.data_view(data);
        req.content_type(content_type);
        return *this;
    }

    auto client::request::upload(
        const std::filesystem::path& path,
        const media_type& content_type
    ) -> request& {
        req.upload(path);
        req.content_type(content_type);
        return *this;
    }
}
