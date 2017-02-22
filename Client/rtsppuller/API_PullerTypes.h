/**
 * @file TypesDefine.h
 * @brief  types define
 * @author K.D, kofera.deng@gmail.com
 * @version 1.0
 * @date 2015-11-11
 */

#ifndef API_PULLER_TYPES_H
#define API_PULLER_TYPES_H

#ifdef _WIN32
#define _API __declspec(dllexport)
#define _APICALL __stdcall
#else
#define _API
#define _APICALL
#endif

#define RTSP_Puller_Handler void*

enum 
{
	MC_NoErr			=	0,
};
typedef int MC_Error;


/* 连接类型 */
typedef enum __RTP_CONNECT_TYPE
{
	RTP_OVER_TCP	=	0x01,		/* RTP Over TCP */
	RTP_OVER_UDP					/* RTP Over UDP */
} RTP_ConnectType;


typedef enum __CB_DATA_TYPE
{
	CB_MEDIA_ATTR	=	0x01,
	CB_RTP_DATA		=	0x02,
	CB_PULLER_STATE	=	0x03,
} CBDataType;

typedef struct __RTP_DATA
{
	char*	dataBuf;
	int		bufLen;
} RTPData;

typedef struct __MEDIA_ATTR
{
	unsigned int audioCodec;			/* 音頻編碼类型*/
	unsigned int audioSamplerate;		/* 音頻采样率*/
	unsigned int audioChannel;			/* 音頻通道数*/
} MediaAttr;

typedef struct __PULLER_STATE
{
	int resultCode;						/* positive: rtsp error code; negative: is the standard "errno" */
	char* resultString;
} PullerState;

#endif
