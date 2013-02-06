from distutils.core import setup, Extension

setup(
	name = 'zpu-tools',
	version = '1.0',
	author = 'Martin Helmich',
	author_email = 'martin.helmich@hs-osnabrueck.de',
	description = 'A toolset for controlling a Raggedstone ZPU board.',
	package_dir = {'': 'src'},
	py_modules = ['libzputerm'],
	scripts = ['src/zpuload','src/zputerm'],
	requires = ['libzpu (> 1.0)'])
