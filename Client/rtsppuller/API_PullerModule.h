
/**
 * @file API_PullerModule.h
 * @brief  Puller API Define
 * @author K.D, kofera.deng@gmail.com
 * @version 1.0
 * @date 2015-11-11
 */

#ifndef API_PULLER_MODULE_H
#define API_PULLER_MODULE_H

#include "API_PullerTypes.h"


/**
 * @brief  业务层回调函数定义 
 *
 * @param dataBuf  需要处理的媒体数据,此处默认为RTP包 
 * @param bufLen   数据长度
 * @param obj      回调函数的处理对象,如果有.
 *
 * @return   
 */


typedef int (_APICALL *PullerCallback)(CBDataType dataType, void* data,  void* obj);

#ifdef __cplusplus
extern "C"
{
#endif
	
	
	/**
	 * @brief  RTSP_Puller_GetErrcode 
	 *			获取最后一次错误的错误码
	 * @return  返回对应错误码
	 */
	_API int _APICALL RTSP_Puller_GetErrcode();


	/**
	 * @brief  RTSP_Puller_Create 
	 *			创建拉取流句柄
	 * @return  返回处理结果: NULL 为创建失败 
	 */
	_API RTSP_Puller_Handler _APICALL RTSP_Puller_Create();


	/**
	 * @brief  RTSP_Puller_SetCallback 
	 *		设置数据处理回调函数, RTSP_Puller_StartStream 正常返回后触发
	 * @param handler		拉取流句柄
	 * @param cb			处理回调函数
	 * @param cbParam		回調函数传入参数
	 * @return				返回处理结果
	 */
	_API int _APICALL RTSP_Puller_SetCallback(RTSP_Puller_Handler handler, PullerCallback cb,\
			void* cbParam);


	/**
	 * @brief  RTSP_Puller_StartStream 
	 *		开始拉取流访问
	 * @param handler		拉取流句柄
	 * @param url			访问流RTSP URL
	 * @param connType		RTP 数据访问类型:tcp or udp
	 * @param username		用户名
	 * @param password		用户访问密码
	 * @param reconn		连接次数: 0 or nonzero, 0 表示循环重试
	 * @param retRtpPkt		返回数据类型: 0 表示RTP包, 1 表示帧数据
	 *
	 * @return  返回处理结果 
	 */
	_API int _APICALL RTSP_Puller_StartStream(RTSP_Puller_Handler handler, const char* url, \
			RTP_ConnectType connType, const char* username, const char* password, int reconn, int retRtpPkt);


	/**
	 * @brief  RTSP_Puller_CloseStream 
	 *		结束拉取流访问
	 * @param handler 拉取流句柄
	 *
	 * @return  返回处理结果 
	 */
	_API int _APICALL RTSP_Puller_CloseStream(RTSP_Puller_Handler handler);


	/**
	 * @brief  RTSP_Puller_Release 
	 *		拉取流句柄资源释放
	 * @param handler  拉取流句柄
	 *
	 * @return  返回处理结果 
	 */
	_API int _APICALL RTSP_Puller_Release(RTSP_Puller_Handler handler);

#ifdef __cplusplus
}
#endif

#endif 
