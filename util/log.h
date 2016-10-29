//+------------------------------------------------------------------------------------+
//+FileName : Log.h
//+Author :	wanglx
//+Version : 0.0.1
//+Date : 201-04-28
//+Description : 
//+------------------------------------------------------------------------------------+
#pragma once
#include <stdio.h>
#include "pthread.h"

const int MAX_PATH_LENGTH = 255 * 2;
enum LOG_LEVEL
{
	LOG_LEVEL_SYS = 0,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG
};
#define LOG_FILE_OUT	0x01								//just output to log file, with datetime out
#define LOG_STD_OUT		0x02								//just output to std, without datetime out
#define LOG_ALL_OUT		(LOG_FILE_OUT | LOG_STD_OUT)		//both output to log file and std

void log_printf(const int level,const char* str,...);
void log_init(const char* sPath);


//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CLog
{
public:
	~CLog(void);
static CLog& GetIntance();
void Init(const char* sPath);
void UnInit();
void WriteLogEx(const int outTarget,const int level,const char* str,...);
void WriteLog(const int outTarget,const int level,const char* str);
void SetLogLevel(int nLevel) { m_nLevel = nLevel; };

protected:
	FILE* GetFilePointer(char* path);
	long GetFileSize( FILE *fp );
	long GetFileSizeEx( char* path );
	void CheckFile();
private:
	CLog(void);
	CLog& operator=(const CLog&);
	static void* ThreadStdWriteProc(void* param);
	static void* ThreadFileWriteProc(void* param);


	FILE* 		m_fp;
	int 		m_nLevel;
	int 		m_nFileIndex;
	bool 		m_bStopLog;
	bool 		m_bFileFirstLine;
	char* 		m_pStdBuffer;
	char* 		m_pFileBuffer;
	int 		m_nStdBufferWritePos;
	int 		m_nFileBufferWritePos;
	char 		m_szFilePath[MAX_PATH_LENGTH]; 
	pthread_t 	m_threadIDStd;
	pthread_t 	m_threadIDFile;
	pthread_mutex_t m_lock;
};
