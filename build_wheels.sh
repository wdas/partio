#!/bin/bash
set -e -u -x

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    # ignore the pp37
    if [[ $PYBIN != *"pp37-pypy37_pp73"* ]]; then

        "${PYBIN}/python" setup.py bdist_wheel
    fi
done

# Bundle external shared libraries into the wheels
for whl in dist/*.whl; do
    auditwheel repair -w dist $whl
    rm $whl
done

# Install packages and test
for PYBIN in /opt/python/*/bin; do
    if [[ $PYBIN != *"pp37-pypy37_pp73"* ]]; then
        "${PYBIN}/pip" install partio --no-index -f /io/dist
        # very simple test import and create
        "${PYBIN}/python" -c "import sys; import partio; partio.create(); print('test passed for ' + sys.version)"
    fi
done

# clean up
rm -rf partio.egg-info build

echo "Wheels successfully build to ./dist"
