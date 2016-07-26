set(NAME consumer)
add_executable(${NAME} main.cpp)

target_link_libraries(${NAME}
    rabbitmq_client
    ${RABBITMQ_LIBRARIES}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)