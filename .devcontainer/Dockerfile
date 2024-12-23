FROM livekit/gstreamer:1.24.5-dev-amd64

# 设置环境变量避免交互式界面
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get -y install --no-install-recommends build-essential meson ninja-build vim gdb cmake make valgrind
#     git libglib2.0-dev libsoup2.4-dev libdw-dev libgtk-3-dev python3-pip libasound2-dev cdparanoia \
#     libvisual-0.4-dev libsdl2-2.0 libsdl2-dev libsdl2-mixer-dev libsdl2-image-dev libsdl2-ttf-dev  libsdl2-gfx-dev \
#     qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qtcreator libbz2-dev nasm yasm gettext
# #
# 生产阶段请取消以下注释
# && apt-get autoremove -y \
# && apt-get clean -y \
# && rm -rf /var/lib/apt/lists/*
# 重置 DEBIAN_FRONTEND 环境变量
ENV DEBIAN_FRONTEND=dialog

ARG WORKSPACE