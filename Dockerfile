ARG VERSION=3.1.4
# FROM fluent/fluent-bit:${VERSION}
FROM fluent/fluent-bit:${VERSION}-debug AS builder
ARG VERSION

WORKDIR /build

# build on fluent-bit for supporting headers and libs
RUN git clone --depth 1 --branch v${VERSION} https://github.com/fluent/fluent-bit.git && \
    # rather dirty, will build in root dir rather than build dir so that built header files
    # will be available to the plugin on fluent-bit include paths
    cd fluent-bit && \
    cmake .

COPY in_vdisk fluent-bit-disk/in_vdisk
COPY CMakeLists.txt fluent-bit-disk/CMakeLists.txt

# build on fluent-bit-disk
RUN cd fluent-bit-disk && \
    mkdir -p build && \
    cd build && \
    cmake -DFLB_SOURCE=/build/fluent-bit/ -DPLUGIN_NAME=in_vdisk .. && \
    make && \
    # we should now have some shared objects to copy into the release image
    find ./ -name *.so

FROM fluent/fluent-bit:${VERSION}
# FROM fluent/fluent-bit:${VERSION}-debug
# register the new shared object in the plugins configuration
COPY example-plugins.conf fluent-bit/etc/plugins.conf
# and copy the shared object into the release image
COPY --from=builder /build/fluent-bit-disk/build/flb-in_vdisk.so /fluent-bit/plugins/flb-in_vdisk.so
