#include <http/media_type.hpp>

namespace http {
    media_type::media_type(const char* str) :
        media_type(std::string_view(str))
    {}

    media_type::media_type(std::string_view type) {
        auto begin = type.begin();
        auto it = begin;
        const auto end = type.end();

        while (it != end && *it != '/') ++it;
        auto distance = std::distance(begin, it);

        if (it == end || distance > max_size) {
            throw invalid_media_type(type);
        }

        type_len = distance;
        begin = ++it; // Consume the '/'.

        while (it != end && *it != ';') ++it;

        distance = std::distance(begin, it);
        if (distance > max_size) throw invalid_media_type(type);

        subtype_len = distance;

        if (it != end) {
            begin = ++it; // Consume the ';'.
            if (it != end && *it == ' ') begin = ++it;

            while (it != end && *it != '=') ++it;
            distance = std::distance(begin, it);
            if (it == end || distance > max_size) {
                throw invalid_media_type(type);
            }

            param_name_len = distance;

            ++it; // Consume the '='.
            if (it == end) throw invalid_media_type(type);

            distance = std::distance(it, end);
            if (distance > max_size) throw invalid_media_type(type);
            param_value_len = distance;
        }

        storage.resize(
            type_len +
            subtype_len +
            param_name_len +
            param_value_len +
            1 + // '/'
            (param_name_len == 0 ? 0 : 2) // ';' and '='
        );

        distance = type_len + subtype_len + 1;

        for (auto i = 0ul; i < distance; ++i) {
            storage[i] = std::tolower(type[i]);
        }

        if (param_name_len > 0) {
            storage[distance++] = ';';

            auto i = distance;
            auto j = distance;
            if (type[j] == ' ') ++j;

            distance = storage.size();

            for (; i < distance; ++i, ++j) {
                storage[i] = std::tolower(type[j]);
            }
        }
    }

    auto media_type::operator==(
        const media_type& other
    ) const noexcept -> bool {
        return storage == other.storage;
    }

    auto media_type::parameter() const noexcept -> std::optional<name_value> {
        if (param_name_len == 0) return std::nullopt;

        const auto* const data = storage.data() + type_len + subtype_len;

        return name_value(
            {data, param_name_len},
            {data + param_name_len, param_value_len}
        );
    }

    auto media_type::str() const noexcept -> std::string_view {
        return storage;
    }

    auto media_type::subtype() const noexcept -> std::string_view {
        return {storage.data() + type_len + 1, subtype_len};
    }

    auto media_type::type() const noexcept -> std::string_view {
        return {storage.data(), type_len};
    }

    namespace media {
        auto json() noexcept -> const media_type& {
            static const media_type type = "application/json";
            return type;
        }

        auto octet_stream() noexcept -> const media_type& {
            static const media_type type = "application/octet-stream";
            return type;
        }

        auto plain_text() noexcept -> const media_type& {
            static const media_type type = "text/plain";
            return type;
        }

        auto utf8_text() noexcept -> const media_type& {
            static const media_type type = "text/plain; charset=UTF-8";
            return type;
        }
    }
}
