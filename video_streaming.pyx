from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint cimport uint8_t

from libc.stdlib cimport free
from cpython cimport PyObject, Py_INCREF

import numpy as np

cimport numpy as np

np.import_array()

cdef extern from "./src/VideoEncoder.h":
    cdef cppclass VideoEncoder:
        VideoEncoder(int, int, int, int, string, int, int, int) except +
        vector[uint8_t] encodeFrame(uint8_t *, int, int, int) except +
    
cdef class PyVideoEncoder:
    
    """
    Encodes video

    ...

    Methods
    -------
        encodeFrame(frame)
            Accepts a frame, outputs a byte message. Note that there's no one-to-one correspondence between frames and messages.
    """
    
    cdef VideoEncoder * thisptr
    
    def __cinit__(self, int width, int height, int bitrate, string codec_name, int fps, int gop_size, int max_b_frames):
        
        """
        Parameters
        ----------
        width : int
            Frame width
        height : int
            Frame height
        bitrate : int
            Video bitrate. The lower is the bitrate, the higher is compression.
        codec_name: str
            The name of the codec to use. Note that the encoder and the decoder must use the exact same codec. Tested with libx264 and mpeg1video.
        fps: int
            Frames per second
        gop_size: int
            Group of pictures size. Please see https://en.wikipedia.org/wiki/Group_of_pictures
        max_b_frames: int
            Maximum quantity of consecutive B frames. Please see https://en.wikipedia.org/wiki/Video_compression_picture_types
        """
        
        self.thisptr = new VideoEncoder(width, height, bitrate, 0, codec_name, fps, gop_size, max_b_frames)
        
    def __dealloc__(self):
        
        del self.thisptr
        
    def encodeFrame(self, frame):
        
        """
        Parameters
        ----------
        frame: np.array
            RGB frame
            
        Returns
        -------
        bytes
            a byte message
        """
        
        cdef np.ndarray[np.uint8_t, ndim=3, mode = 'c'] np_buff = np.ascontiguousarray(frame, dtype = np.uint8)
        cdef uint8_t * im_buff = <uint8_t *> &np_buff[0, 0, 0]
        cdef vector[uint8_t] frame_bytes
        frame_bytes = self.thisptr.encodeFrame(im_buff, frame.strides[0], frame.strides[1], frame.strides[2])
        return bytes(frame_bytes)
    
cdef extern from "./src/VideoDecoder.h":
    cdef struct VideoFrame:
        int width
        int height
        int nc
        int pts
        uint8_t * data
        
cdef extern from "./src/VideoDecoder.h" nogil:
    cdef cppclass VideoDecoder:
        VideoDecoder(int, int, string) except +
        void supplyBytes(uint8_t *, int) except +
        VideoFrame getFrame() except +


cdef class ArrayWrapper:

    cdef uint8_t* data_ptr
    cdef int width
    cdef int height
    cdef int nc

    cdef set_data(self, int width, int height, int nc, uint8_t* data_ptr):
        
        self.data_ptr = data_ptr
        self.width = width
        self.height = height
        self.nc = nc

    def __array__(self):

        cdef np.npy_intp shape[3]
        shape[0] = <np.npy_intp> self.height
        shape[1] = <np.npy_intp> self.width
        shape[2] = <np.npy_intp> self.nc
        
        ndarray = np.PyArray_SimpleNewFromData(3, shape, np.NPY_UINT8, <uint8_t *> self.data_ptr)
        
        return ndarray

    def __dealloc__(self):
        free(<uint8_t*>self.data_ptr)
        
cdef class PyVideoDecoder:
    
    """
    Decodes video

    ...

    Methods
    -------
        supplyBytes(data)
            Accepts a byte message
        getFrame()
            Blocks until a frame is available. Returns a frame and a presentation timestamp
    """
    
    cdef VideoDecoder * thisptr
    
    def __cinit__(self, int width, int height, string codec_name):
        
        """
        Parameters
        ----------
        width : int
            Frame width
        height : int
            Frame height
        codec_name : str
            The name of the codec to use. Note that the encoder and the decoder must use the exact same codec. Tested with h264 and mpeg1video.
        """
        
        self.thisptr = new VideoDecoder(width, height, codec_name)
        
    def __dealloc__(self):
        
        del self.thisptr
        
    def supplyBytes(self, data):
        
        """
        Parameters
        ----------
        data: bytes
            Bytes message
        """
        
        size = len(data)
        cdef uint8_t * data_ptr = data
        self.thisptr.supplyBytes(data_ptr, size)
        
    def getFrame(self):
        
        """
        Blocks until a frame is available
        
        Returns
        -------
        ndarray
            Decoded frame
        pts
            Presentation timestamp
        """
        
        with nogil:
            video_frame = self.thisptr.getFrame()
        cdef np.ndarray ndarray
        array_wrapper = ArrayWrapper()
        array_wrapper.set_data(video_frame.width, video_frame.height, video_frame.nc, video_frame.data)
        ndarray = np.array(array_wrapper, copy=False)        
        ndarray.base = <PyObject*> array_wrapper
        Py_INCREF(array_wrapper)
        return ndarray, video_frame.pts
        