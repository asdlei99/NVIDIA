#pragma once
#include<Windows.h>
#include"dynlink_cuda.h"
#include"dynlink_nvcuvid.h"

#define DEF_OUT_LOG(str,str2)	\
{								\
	char buf[1024] = { 0 };		\
	sprintf(buf, str, str2);	\
	OutputDebugString(buf);		\
}

class CDecoder
{
public:
	CDecoder();
	~CDecoder();

public:
	HMODULE		m_hHandleDriver;
	CUcontext	m_cuContext;
	CUvideoctxlock	m_cuVideoctxlock;
	CUvideodecoder  m_cuVideoDecoder;
	CUvideoparser m_cuVideoParser;
	CUstream	  m_cuStream;

	BYTE          *m_pFrameYUV[4];
	bool		  m_bFirstFlag;
	int			m_iHeight;
	int			m_iWigth;
	//FILE        *m_pYUVFILE;

private:
	bool createvideoparse();

	bool getdatafromsurface(CUVIDPARSERDISPINFO *p);

	void SaveFrameAsYUV(unsigned char *pdst,
		const unsigned char *psrc,
		int width, int height, int pitch);
public:
	bool Open();
	
	bool SetParameters(int iHeight, int iWidth);
		
	bool Decode(const char *src, int iSize, int Flag);
	
	bool Close();

	static int CUDAAPI HandleVideoSequence(void *p, CUVIDEOFORMAT *cuvidformat);

	static int CUDAAPI HandlePictureDecode(void *p, CUVIDPICPARAMS *cuvidpicparams);

	static int CUDAAPI HandlePictureDisplay(void *, CUVIDPARSERDISPINFO *);
};

