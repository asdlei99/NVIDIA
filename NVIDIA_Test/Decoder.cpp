#include "Decoder.h"

CDecoder::CDecoder()
{
	m_hHandleDriver = nullptr;
	m_cuContext = nullptr;
	m_cuVideoctxlock = nullptr;
	m_cuVideoDecoder = nullptr;
	m_cuVideoParser = nullptr;
	m_cuStream = nullptr;
	//m_pYUVFILE = nullptr;
	m_bFirstFlag = true;
}


CDecoder::~CDecoder()
{
}

bool CDecoder::Open()
{
	CUresult cuResult;

	int iVersin = 0;
	int iDeviceCount = 0;

	cuResult = cuInit(0, __CUDA_API_VERSION, m_hHandleDriver);
	cuResult = cuvidInit(0);

	//获取设备版本
	cuResult = cuDriverGetVersion(&iVersin);

	//获取设备个数
	cuResult = cuDeviceGetCount(&iDeviceCount);
	
	//获取设备名称
	CUdevice cdDevice;
	char chDeivceName[256] = {0};
	for (int i =0;i<iDeviceCount;i++)
	{
		cuResult = cuDeviceGet(&cdDevice,i);
		cuResult = cuDeviceGetName(chDeivceName, 256, cdDevice);
	}

	//通过device创建context
	cuResult = cuCtxCreate(&m_cuContext, CU_CTX_SCHED_AUTO, cdDevice);

	//创建Contex对应的Lock
	cuResult = cuvidCtxLockCreate(&m_cuVideoctxlock, m_cuContext);

	//创建Sream
	cuResult = cuStreamCreate(&m_cuStream, 0);

	return true;
}

bool CDecoder::Close()
{
	cuvidDestroyDecoder(m_cuVideoDecoder);
	
	cuStreamDestroy(m_cuStream);

	cuvidCtxLockDestroy(m_cuVideoctxlock);

	cuCtxDestroy(m_cuContext);

	cuMemFreeHost((void *)m_pFrameYUV[0]);
	cuMemFreeHost((void *)m_pFrameYUV[1]);
	cuMemFreeHost((void *)m_pFrameYUV[2]);
	cuMemFreeHost((void *)m_pFrameYUV[3]);
	m_pFrameYUV[0] = NULL;
	m_pFrameYUV[1] = NULL;
	m_pFrameYUV[2] = NULL;
	m_pFrameYUV[3] = NULL;

	return true;
}
bool CDecoder::Decode(const char *src,int iSize,int Flag)
{
	CUVIDSOURCEDATAPACKET packet = { 0 };

	packet.flags = CUVID_PKT_TIMESTAMP;
	packet.payload = (const unsigned char*)src;
	packet.payload_size = iSize;
	packet.timestamp = 0;

	CUresult cuResult = cuvidParseVideoData(m_cuVideoParser, &packet);

	return true;
}
int CUDAAPI CDecoder::HandleVideoSequence(void *p, CUVIDEOFORMAT *cuvidformat)
{
	cudaVideoCodec codec = cuvidformat->codec;
	int iH = cuvidformat->coded_height;
	int iW = cuvidformat->coded_width;
	cudaVideoChromaFormat format = cuvidformat->chroma_format;

	return 0;
}
int CUDAAPI CDecoder::HandlePictureDecode(void *p, CUVIDPICPARAMS *cuvidpicparams)
{
	CDecoder *dlg = reinterpret_cast<CDecoder *>(p);

	cuvidDecodePicture(&dlg->m_cuVideoDecoder, cuvidpicparams);

	return 0;
}
int CUDAAPI CDecoder::HandlePictureDisplay(void *p, CUVIDPARSERDISPINFO *cuvidparserdispinfo)
{
	CDecoder *dlg = reinterpret_cast<CDecoder *>(p);
	
	dlg->getdatafromsurface(cuvidparserdispinfo);

	return 0;
}
bool CDecoder::getdatafromsurface(CUVIDPARSERDISPINFO *oDisplayInfo)
{
	int distinct_fields = 1;
	if (!oDisplayInfo->progressive_frame && oDisplayInfo->repeat_first_field <= 1) {
		distinct_fields = 2;
	}
	CUdeviceptr  pDecodedFrame[2] = { 0, 0 };

	for (int active_field = 0; active_field < distinct_fields; active_field++)
	{

		CUVIDPROCPARAMS oVideoProcessingParameters;
		memset(&oVideoProcessingParameters, 0, sizeof(CUVIDPROCPARAMS));

		oVideoProcessingParameters.progressive_frame = oDisplayInfo->progressive_frame;
		oVideoProcessingParameters.second_field = active_field;
		oVideoProcessingParameters.top_field_first = oDisplayInfo->top_field_first;
		oVideoProcessingParameters.unpaired_field = (distinct_fields == 1);

		unsigned int nDecodedPitch = 0;

		//
		cuvidMapVideoFrame(m_cuVideoDecoder, 
							oDisplayInfo->picture_index, 
							&pDecodedFrame[active_field], 
							&nDecodedPitch, 
							&oVideoProcessingParameters);

		//申请存放数据内存
		if (m_bFirstFlag)
		{
			m_bFirstFlag = false;
			CUresult result;
			result = cuMemAllocHost((void **)&m_pFrameYUV[0], (nDecodedPitch * m_iHeight + nDecodedPitch*m_iHeight / 2));
			result = cuMemAllocHost((void **)&m_pFrameYUV[1], (nDecodedPitch * m_iHeight + nDecodedPitch*m_iHeight / 2));
			result = cuMemAllocHost((void **)&m_pFrameYUV[2], (nDecodedPitch * m_iHeight + nDecodedPitch*m_iHeight / 2));
			result = cuMemAllocHost((void **)&m_pFrameYUV[3], (nDecodedPitch * m_iHeight + nDecodedPitch*m_iHeight / 2));
		}
		//数据拷贝
		cuMemcpyDtoHAsync(m_pFrameYUV[active_field], pDecodedFrame[active_field], (nDecodedPitch * m_iHeight * 3 / 2), m_cuStream);

		cuvidUnmapVideoFrame(m_cuVideoDecoder, pDecodedFrame[active_field]);

		//SaveFrameAsYUV(m_pFrameYUV[active_field + 2], m_pFrameYUV[active_field], m_iWigth, m_iHeight, nDecodedPitch);
	}
	return 0;
}
void CDecoder::SaveFrameAsYUV(unsigned char *pdst,
	const unsigned char *psrc,
	int width, int height, int pitch)
{
	int x, y, width_2, height_2;
	int xy_offset = width*height;
	int uvoffs = (width / 2)*(height / 2);
	const unsigned char *py = psrc;
	const unsigned char *puv = psrc + height*pitch;

	// luma
	for (y = 0; y<height; y++)
	{
		memcpy(&pdst[y*width], py, width);
		py += pitch;
	}

	// De-interleave chroma
	width_2 = width >> 1;
	height_2 = height >> 1;
	for (y = 0; y<height_2; y++)
	{
		for (x = 0; x<width_2; x++)
		{
			pdst[xy_offset + y*(width_2)+x] = puv[x * 2];
			pdst[xy_offset + uvoffs + y*(width_2)+x] = puv[x * 2 + 1];
		}
		puv += pitch;
	}

	//fwrite(pdst, 1, width*height + (width*height) / 2, m_pYUVFILE);
}

bool CDecoder::createvideoparse()
{
	CUVIDPARSERPARAMS oVideoParseParametes;
	
	memset(&oVideoParseParametes,0,sizeof(CUVIDPARSERPARAMS));

	oVideoParseParametes.CodecType = cudaVideoCodec_H264;
	oVideoParseParametes.ulMaxDisplayDelay = 1;
	oVideoParseParametes.ulMaxNumDecodeSurfaces = 20;
	oVideoParseParametes.pUserData = this;
	
	oVideoParseParametes.pfnSequenceCallback = HandleVideoSequence;    // Called before decoding frames and/or whenever there is a format change
	oVideoParseParametes.pfnDecodePicture = HandlePictureDecode;    // Called when a picture is ready to be decoded (decode order)
	oVideoParseParametes.pfnDisplayPicture = HandlePictureDisplay;   // Called whenever a picture is ready to be displayed (display order)

	CUresult oResult = cuvidCreateVideoParser(&m_cuVideoParser, &oVideoParseParametes);

	return true;
}
bool CDecoder::SetParameters(int iHeight,int iWidth)
{
	CUVIDDECODECREATEINFO   oVideoDecodeCreateInfo_;

	m_iHeight = iHeight;
	m_iWigth = iWidth;
	memset(&oVideoDecodeCreateInfo_, 0, sizeof(CUVIDDECODECREATEINFO));

	oVideoDecodeCreateInfo_.CodecType = cudaVideoCodec_H264;
	oVideoDecodeCreateInfo_.ChromaFormat = cudaVideoChromaFormat_420;
	oVideoDecodeCreateInfo_.OutputFormat = cudaVideoSurfaceFormat_NV12;
	oVideoDecodeCreateInfo_.ulHeight = iHeight;
	oVideoDecodeCreateInfo_.ulWidth = iWidth;
	oVideoDecodeCreateInfo_.ulTargetHeight = iHeight;
	oVideoDecodeCreateInfo_.ulTargetWidth = iWidth;
	oVideoDecodeCreateInfo_.ulCreationFlags = cudaVideoCreate_PreferCUVID;
	oVideoDecodeCreateInfo_.ulNumDecodeSurfaces = 20;
	oVideoDecodeCreateInfo_.ulNumOutputSurfaces = 2;
	oVideoDecodeCreateInfo_.vidLock = m_cuVideoctxlock;

	// Limit decode memory to 24MB (16M pixels at 4:2:0 = 24M bytes)
	// Keep atleast 6 DecodeSurfaces
	while (oVideoDecodeCreateInfo_.ulNumDecodeSurfaces > 6 && oVideoDecodeCreateInfo_.ulNumDecodeSurfaces * m_iHeight * m_iWigth > 16 * 1024 * 1024)
	{
		oVideoDecodeCreateInfo_.ulNumDecodeSurfaces--;
	}

	oVideoDecodeCreateInfo_.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;


	CUresult cuResult = cuvidCreateDecoder(&m_cuVideoDecoder, &oVideoDecodeCreateInfo_);

	createvideoparse();

	return true;
}