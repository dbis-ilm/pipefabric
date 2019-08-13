FROM alpine:latest
MAINTAINER Kai-Uwe Sattler <kus@tu-ilmenau.de>

# Installing dependencies
RUN apk update && apk add --no-cache \
      cmake \
      git \
      g++ \
      boost-dev \
      zeromq-dev \
      make \
      sudo && \
    wget -P /usr/include/ https://raw.githubusercontent.com/zeromq/cppzmq/master/zmq.hpp && \
    # Cleaning up
    rm -rf /usr/share/doc && \
    rm -rf /usr/share/man/?? && \
    rm -rf /usr/share/man/??_*

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
    git clone https://dbgit.prakinf.tu-ilmenau.de/code/pfabric.git
# This costs too much space for the image
#    cd pfabric && mkdir build && cd build && \
#    cmake ../src && \
#    make VERBOSE=1 && make test
