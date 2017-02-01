cmake_minimum_required(VERSION 2.8)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})

add_definitions("-Wall -Os -march=native") # <- gcc専用?　
# add_definitions("-Wall -O") # <- 一般的?
add_executable(awin
  awin.c
  mywin.c
  mymem.c
)
# target_link_libraries(awin X11)
target_link_libraries(awin ${X11_LIBRARIES})   # 実験したら単純に X11 を指定した方が少しだけ awin が小さくなった(48バイト)。しかし、Freebsdでは上手く行かなかった
# target_link_libraries(awin m)                  # mywin.cでlog10()を使うなら復活させる必要あり
INSTALL(TARGETS awin RUNTIME DESTINATION bin)
