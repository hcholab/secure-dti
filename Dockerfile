# hadolint global ignore=DL3006,DL3013,DL3018,DL3041,DL3059

# -------------------- base -------------------- #
FROM redhat/ubi9-minimal AS base

SHELL ["/bin/bash", "-euo", "pipefail", "-c"]

RUN echo install_weak_deps=0 >> /etc/dnf/dnf.conf && \
    curl -O https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm && \
    rpm -ivh ./*.rpm && \
    rm -f ./*.rpm && \
    microdnf upgrade -y && \
    microdnf install -y \
        clang \
        git-core \
        gmp-devel \
        gzip \
        libsodium-devel \
        openssl-devel \
        perl \
        python \
        python-pip \
        tar \
    && microdnf clean all && \
    pip install --no-cache-dir \
        matplotlib \
        numpy \
        scikit-learn

WORKDIR /ntl
RUN curl -so- https://libntl.org/ntl-10.3.0.tar.gz | tar -C /ntl -zxvf- --strip-components=1
COPY mpc/code/NTL_mod/ZZ.h /ntl/include/NTL/
COPY mpc/code/NTL_mod/ZZ.cpp /ntl/src/

ARG MARCH=native

WORKDIR /ntl/src
RUN ./configure NTL_THREAD_BOOST=on CXXFLAGS="-g -O2 -march=${MARCH}" && \
    make "-j$(nproc)" all && \
    make install

WORKDIR /build
COPY mpc mpc

WORKDIR /build/mpc/code
RUN sed -i "s|^CPP.*$|CPP = /usr/bin/clang++|g" Makefile && \
    sed -i "s|^INCPATHS.*$|INCPATHS = -I/usr/local/include|g" Makefile && \
    sed -i "s|^LDPATH.*$|LDPATH = -L/usr/local/lib|g" Makefile && \
    sed -i "s|-march=native|-march=${MARCH} -maes|g" Makefile && \
    sed -i "s|c++11|c++14|g" Makefile && \
    sed -i '5i#include <stdint.h>' param.h && \
    make "-j$(nproc)" && \
    rm -rf build include lib

WORKDIR /build
COPY . .
ENTRYPOINT ["./demo.sh"]
