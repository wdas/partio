#!/usr/bin/python

import os, dshell

from pathfinder import build
_opts = build.Options(absolute=True)

shell = dshell.create()
shell.export('PARTIO', os.getcwd())

# Add to paths
test_dir = build.test('partio', opts=_opts)
shell.prepend('PATH', test_dir)
shell.prepend('PYTHONPATH', test_dir)
