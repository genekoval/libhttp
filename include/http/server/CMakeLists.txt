target_sources(http PUBLIC FILE_SET HEADERS FILES
    error.hpp
    handler.hpp
    method_router.hpp
    node.hpp
    request.hpp
    response.hpp
    router.hpp
    server.hpp
    session.hpp
    ssl.hpp
    stream.hpp
)

add_subdirectory(extractor)
add_subdirectory(response)
