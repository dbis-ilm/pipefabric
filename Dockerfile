FROM alpine:latest
MAINTAINER Kai-Uwe Sattler <kus@tu-ilmenau.de>

# Installing dependencies
RUN apk update && apk add --no-cache \
      autoconf \
      bash \
      cmake \
      fts-dev \
      git \
      g++ \
      boost-dev \
      zeromq-dev \
      make \
      ndctl-dev \
      sudo && \
    wget -P /usr/include/ https://raw.githubusercontent.com/zeromq/cppzmq/master/zmq.hpp

# PMDK
RUN git clone https://github.com/pmem/pmdk && \
    cd pmdk && git checkout tags/1.9.1 && \
    export DOC=n && make BUILD_EXAMPLES=n EXTRA_CFLAGS="-Wno-error" EXTRA_LIBS="-lfts" && make install prefix=/usr/local && \
    cd ..

RUN git clone https://github.com/pmem/libpmemobj-cpp && \
    mkdir libpmemobj-cpp/build && cd libpmemobj-cpp/build && \
    git checkout tags/1.11 && \
    cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_DOC=OFF \
             -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_INSTALL_LIBDIR=lib && make && make install && \
    cd ../..

# Cleaning up
RUN rm -rf pmdk && \
    rm -rf /usr/share/doc && \
    rm -rf /usr/local/share/doc && \
    rm -rf /usr/share/man/?? && \
    rm -rf /usr/local/share/man/?? && \
    rm -rf /usr/share/man/??_* && \
    rm -rf /usr/local/share/man/??_*

# Add user and allow sudo
ENV USER pf
ENV USERPASS pfpass
RUN adduser -S $USER && \
    echo "$USER:$USERPASS" | chpasswd && \
    echo "${USER}  ALL=(root) ALL" >> /etc/sudoers
USER $USER
WORKDIR /home/$USER


# Download and build PipeFabric
RUN git config --global http.sslverify false && \
    git clone https://dbgit.prakinf.tu-ilmenau.de/code/pfabric.git && \
    cd pfabric && mkdir build && cd build && \
    cmake ../src
# This costs too much space for the image
#    make VERBOSE=1 && make test
