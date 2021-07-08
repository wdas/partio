FROM quay.io/pypa/manylinux2014_x86_64:latest
RUN (yum install cmake swig freeglut-devel zlib-devel -y)
ADD . / io/
CMD (cd io && \
    /opt/python/cp36-cp36m/bin/python setup.py bdist_wheel && auditwheel repair dist/partio-1.0.0-cp36-cp36m-linux_x86_64.whl && \
    /opt/python/cp37-cp37m/bin/python setup.py bdist_wheel && auditwheel repair dist/partio-1.0.0-cp37-cp37m-linux_x86_64.whl &&  \
    /opt/python/cp38-cp38/bin/python setup.py bdist_wheel && auditwheel repair dist/partio-1.0.0-cp38-cp38-linux_x86_64.whl &&  \
    /opt/python/cp39-cp39/bin/python setup.py bdist_wheel && auditwheel repair dist/partio-1.0.0-cp39-cp39-linux_x86_64.whl )