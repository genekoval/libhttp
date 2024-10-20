target_sources(http PRIVATE
    client.cpp
    file.cpp
    init.cpp
    media_type.cpp
    request.cpp
    response.cpp
    session.cpp
    stream.cpp
    string.cpp
    url.cpp
)

if(PROJECT_TESTING)
    target_sources(http.test PRIVATE
        http.test.cpp
        url.test.cpp
    )
endif()

add_subdirectory(server)
