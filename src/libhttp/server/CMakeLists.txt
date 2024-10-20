target_sources(http PRIVATE
    method_router.cpp
    request.cpp
    router.cpp
    server.cpp
    session.cpp
    ssl.cpp
    stream.cpp
)

add_subdirectory(extractor)
