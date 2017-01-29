cmake_minimum_required(VERSION 2.8)
project(awin C)

set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 2)

# make の出力を強制 (デバッグ用)
set(CMAKE_VERBOSE_MAKEFILE 1)


# サブディレクトリ (ソースファイル格納) を追加
add_subdirectory(src)
add_subdirectory(man)

# インクルードファイルのパスを指定
# include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions("-Wall -Os -march=native") # <- gcc専用?　
# add_definitions("-Wall -O") # <- 一般的?
