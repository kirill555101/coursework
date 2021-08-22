cmake_minimum_required(VERSION 3.16)

project(guron)

find_package( Boost COMPONENTS log log_setup REQUIRED )

include_directories(${Boost_INCLUDE_DIR} project/config/include project/http/include project/logs/include project/server/include project/workers/include)

link_directories(${Boost_LIBRARY_DIR})

aux_source_directory(project/config/src CONFIG_SRC)
aux_source_directory(project/http/src HTTP_SRC)
aux_source_directory(project/logs/src LOG_SRC)
aux_source_directory(project/server/src SERVER_SRC)
aux_source_directory(project/workers/src WORKERS_SRC)

add_executable(guron.out ${CONFIG_SRC} ${HTTP_SRC} ${LOG_SRC} ${SERVER_SRC} ${WORKERS_SRC})

target_link_libraries(guron.out ${Boost_LIBRARIES} )
