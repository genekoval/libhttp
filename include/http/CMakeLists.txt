target_sources(http PUBLIC FILE_SET HEADERS FILES
    client.hpp
    error.h
    file.hpp
    http
    init.h
    json.hpp
    media_type.hpp
    parser.hpp
    request.h
    response.h
    session.hpp
    stream.hpp
    string.h
    url.h
)

add_subdirectory(server)
