FROM quay.io/pypa/manylinux2014_x86_64:latest
RUN yum install -y cmake swig freeglut-devel zlib-devel 
ADD . / io/
WORKDIR /io
CMD ./build_wheels.sh
