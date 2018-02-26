cmake_minimum_required(VERSION 2.6)
project (sllin_app)

#define Sources and headers
set (LINAPP_SRC   sllin_app.c)
set (LINAPP_INC   sllin_app.h)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -O3")

#compiling step
add_executable(sllin_app ${LINAPP_SRC} ${LINAPP_INC})

#link step
target_link_libraries(sllin_app -lpthread)
