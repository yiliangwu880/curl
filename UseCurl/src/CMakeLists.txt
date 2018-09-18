#project name
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) #cmake最低版本需求，不加入此行会受到警告信息
PROJECT(HELLO) #项目名称　　

INCLUDE_DIRECTORIES(system ${PROJECT_SOURCE_DIR}/../dep/include)

# lib directories
LINK_DIRECTORIES(

  #项目引用第三方库
  ${PROJECT_SOURCE_DIR}/../dep/lib
  

)

# 源文件搜索
FILE(GLOB_RECURSE SourceFiles 
        "${PROJECT_SOURCE_DIR}/*.cpp" "${PROJECT_SOURCE_DIR}/*.cc") 
        
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -pthread -lrt -lcrypto -lssl -lz  -lrt")
#SET(CMAKE_CXX_FLAGS "-g -std=c++11 -lcrypto -lssl -lz  -lrt -D_DEBUG -DDEBUG -Wall -Wextra -ffast-math -m64 -Wno-reorder -Wno-invalid-offsetof -Wno-array-bounds -finput-charset=UTF-8  -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated -Wno-missing-field-initializers -Wno-sign-compare -pthread")
ADD_EXECUTABLE(hello ${SourceFiles}) 

# 设置链接库
TARGET_LINK_LIBRARIES(hello libcurl.a )

