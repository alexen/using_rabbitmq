set(NAME rabbitmq_client)

add_library(${NAME}
    src/error.cpp
    src/utils.cpp
    src/simple_client.cpp
)

target_link_libraries(${NAME}
    ${RABBITMQ_LIBRARIES}
    ${Boost_THREAD_LIBRARY}
)