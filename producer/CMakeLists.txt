set(NAME producer)
add_executable(${NAME} main.cpp)

target_link_libraries(${NAME}
    rabbitmq_client
    ${RABBITMQ_LIBRARIES}
)