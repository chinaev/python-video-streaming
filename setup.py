
import numpy
from Cython.Distutils import build_ext

include_dir = '/usr/local/include'
lib_dir = '/usr/local/lib/'


def configuration(parent_package='', top_path=None):
    """ Function used to build our configuration.
    """
    from numpy.distutils.misc_util import Configuration

    # The configuration object that hold information on all the files
    # to be built.
    config = Configuration('', parent_package, top_path)
    config.add_extension('video_streaming',
                         sources=['video_streaming.pyx', './src/VideoEncoder.cpp', './src/VideoDecoder.cpp'],
                         libraries=['avcodec', 'avutil', 'avformat', 'swresample', 'swscale', 'z', 'lzma', 'pthread'],
                         depends=['./src/VideoEncoder.cpp', './src/VideoEncoder.h', './src/VideoDecoder.cpp', './src/VideoDecoder.h'],
                         language = "c++",
                         extra_compile_args=['-std=c++11'],
                         include_dirs=[numpy.get_include(), include_dir],
                         library_dirs= [lib_dir])
    return config


if __name__ == '__main__':
    # Retrieve the parameters of our local configuration
    params = configuration(top_path='').todict()

    # Override the C-extension building so that it knows about '.pyx'
    # Cython files
    params['cmdclass'] = dict(build_ext=build_ext)

    # Call the actual building/packaging function (see distutils docs)
    from numpy.distutils.core import setup
setup(**params)