# NVIDIA

### 说明

SDK：<https://developer.nvidia.com/nvidia-video-codec-sdk>



### 解码

1:加载动态库并进行初始化

2：创建Contex cuCtxCreate

3：设置参数并创建解码器cuvidCreateDecoder

4：创建视频解析器cuvidCreateVideoParser

5：解析视频数据cuvidParseVideoData

### 方法说明

**cuInit**：动态加载nvcuda.dll，并进行函数指针关联.

**cuvidInit**：动态加载nvcuvid.dll，并进行函数指针关联.

**cuCtxCreate**：创建CUContex,用于解码器的创建.

**cuvidCreateVideoParser**：创建CUVideoparser,用于实现三个回调函数,用于实现视频处理的各个流程.

**cuvidParseVideoData**：进行视频解析

**HandleVideoSequence**:是parse解析到序列及图像参数集信息时的回调，其传入的参数为parser解析好的视频参数,可以用于初始化解码器或者重置解码器.

**HandlePictureDecode**：是parser解析到的视频编码数据后的回调函数,其传入的参数为待解码的视频编码数据.需要在该函数中调用解码操作.

**HandlePictureDisplay**：是parser对解码后的数据处理的回调函数,可以在该回调中对已解码数据进行获取.

**cuvidMapVideoFrame**：获取解码后数据地址进行映射

**cuMemAllocHost**：申请内存显存与内存都可以访问.

**cuMemcpyDtoHAsync**：将数据从拷贝到目的地址

**cuvidUnmapVideoFrame**：取消映射