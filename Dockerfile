# Based on cxxdev by Dan Liew <daniel.liew@imperial.ac.uk>

FROM ubuntu:14.04
MAINTAINER Kai-Uwe Sattler <kus@tu-ilmenau.de>
ENV CONTAINER_USER="cxxdev"

RUN apt-get update && apt-get -y upgrade && apt-get -y install wget

RUN apt-get -y --no-install-recommends install \
  aptitude \
  software-properties-common \
  python-software-properties \
  bash-completion \
  build-essential \
  coreutils \
  git-core \
  htop \
  mercurial \
  ncdu \
  ninja-build \
  python \
  python-dev \
  python-pip \
  subversion \
  tmux \
  tree \
  unzip \
  vim

RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update
RUN apt-get -y install gcc-5 g++-5
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1 --slave /usr/bin/g++ g++ /usr/bin/g++-5


# Download and install ZeroMQ
RUN wget https://github.com/zeromq/zeromq4-1/releases/download/v4.1.5/zeromq-4.1.5.tar.gz; \
tar xvzof zeromq-4.1.5.tar.gz && rm zeromq-4.1.5.tar.gz
RUN cd zeromq-4.1.5 && ./configure && make install
RUN wget https://raw.githubusercontent.com/zeromq/cppzmq/master/zmq.hpp && mv zmq.hpp /usr/local/include

# Download and install a more recent version of CMake
RUN wget https://cmake.org/files/v3.6/cmake-3.6.2.tar.gz ; \
tar xvzof cmake-3.6.2.tar.gz && rm cmake-3.6.2.tar.gz
RUN cd cmake-3.6.2 && ./configure && make && make install

# Download and install the most recent version of the Boost libraries
RUN wget https://sourceforge.net/projects/boost/files/boost/1.62.0/boost_1_62_0.tar.gz; \
 tar xvzof boost_1_62_0.tar.gz; \
rm boost_1_62_0.tar.gz;
RUN cd boost_1_62_0 && ./bootstrap.sh --prefix=/usr/local/boost && mkdir /usr/local/boost && ./b2 install

# Add non-root user for container but give it sudo access.
# Password is the same as the username
RUN useradd -m ${CONTAINER_USER} && \
    echo ${CONTAINER_USER}:${CONTAINER_USER} | chpasswd && \
    cp /etc/sudoers /etc/sudoers.bak && \
    echo "${CONTAINER_USER}  ALL=(root) ALL" >> /etc/sudoers
# Make bash the default shell (useful for when using tmux in the container)
RUN chsh --shell /bin/bash ${CONTAINER_USER}
USER ${CONTAINER_USER}

# Download an build PipeFabric
RUN cd /home/${CONTAINER_USER} && \
git clone http://dbgit.prakinf.tu-ilmenau.de/code/pfabric.git && \
cd pfabric && mkdir build && cd build && \
export BOOST_ROOT=/usr/local/boost; \
cmake ../src && make
