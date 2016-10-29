#include "log.h"
#include <assert.h>
#include <sys/stat.h>	/*mkdir*/
#include <errno.h>
#include <unistd.h>		/*access...*/
#include <sys/time.h>
#include <stdarg.h>		/*vsnprintf*/
#include <dirent.h>		/*opendir*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
const char* LOG_LEVEL_CONTENT[] ={"[ SYS ]","[ERROR]","[ WARN]","[ INFO]","[DEBUG]"}; 
const int LOG_STD_BUFFER_SIZE = 5 * 1024 * 1024;
const int LOG_FILE_BUFFER_SIZE = 5 * 1024 * 1024;
const int MAX_LOG_FILE_SIZE = 50 * 1024 * 1024;
const int MAX_FILE_LOG_COUNT = 10;

CLog::CLog(void):
m_fp(NULL),
m_nLevel(4),
m_nFileIndex(0),
m_bStopLog(false),
m_bFileFirstLine(true),
m_pStdBuffer(NULL),
m_pFileBuffer(NULL),
m_nStdBufferWritePos(0),
m_nFileBufferWritePos(0)
{
	pthread_mutex_init(&m_lock,NULL);
}

CLog::~CLog(void)
{
	pthread_mutex_destroy(&m_lock);
}
CLog& CLog::GetIntance()
{
	static CLog instance;
	return instance;
}
void CLog::Init(const char* sPath)
{
	char logDirectory[255] = {0};
	pthread_mutex_lock(&m_lock);
	m_bStopLog = false;
	m_pStdBuffer = (char*)malloc(LOG_STD_BUFFER_SIZE);
	m_pFileBuffer = (char*)malloc(LOG_FILE_BUFFER_SIZE);
	assert(m_pStdBuffer);
	assert(m_pFileBuffer);	
	strcpy(logDirectory,sPath);
	memcpy(m_szFilePath,logDirectory,MAX_PATH_LENGTH);
	pthread_mutex_unlock(&m_lock);
	m_fp = GetFilePointer(logDirectory);
	if (m_fp != NULL)
	{
		printf("create file thread!\n");
		if( 0 != pthread_create(&m_threadIDFile,NULL,ThreadFileWriteProc,this) ) 
		{
			printf("create file write thread failed,errno:%d\n",errno);
		}
	}
	if( 0 != pthread_create(&m_threadIDStd,NULL,ThreadStdWriteProc,this) ) 
	{
		printf("create std write thread failed,errno:%d\n",errno);
	}
}

FILE* CLog::GetFilePointer(char* path)
{
	char szFileName[MAX_PATH_LENGTH] = {0};
	char szTmpPath[MAX_PATH_LENGTH] = {0};
	if (NULL!=path && strlen(path)!=0)
	{
		if( NULL == opendir(path) && 0!= mkdir(path,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) )
		{
			printf("create directory failed!directory:%s,errno:%d\n",path,errno);
		}
		sprintf(szTmpPath,"%s",path);
	}
	while( m_nFileIndex < MAX_FILE_LOG_COUNT )
	{
		sprintf(szFileName,"%s/log_%04d.txt",szTmpPath,m_nFileIndex);
		if ( access(szFileName,0) < 0 || GetFileSizeEx(szFileName) < MAX_LOG_FILE_SIZE )
		{
			break;
		}
		++m_nFileIndex;
	}
	if ( m_nFileIndex >= MAX_FILE_LOG_COUNT )
	{
		m_nFileIndex = 0;
		sprintf(szFileName,"%s/log_%04d.txt",szTmpPath,m_nFileIndex);
		remove(szFileName);
	}
	
	FILE* fp = NULL;
	fp = fopen(szFileName,"a+");
	if ( fp == NULL )
	{
		printf("Open log file %s Failed! Next will not write log to file!errno:%d\n",szFileName,errno);
	}
	return fp;
}

long CLog::GetFileSizeEx( char* path )
{
	int size = 0;
	FILE* fp = NULL;
	fp = fopen(path,"r");
	if ( fp != NULL )
	{
		size = GetFileSize(fp);
		fclose(fp);
	}
	return size;
}

long CLog::GetFileSize( FILE *fp )
{
	long int save_pos;
	long size_of_file;

	/* Save the current position. */
	save_pos = ftell( fp );

	/* Jump to the end of the file. */
	fseek( fp, 0L, SEEK_END );

	/* Get the end position. */
	size_of_file = ftell( fp );

	/* Jump back to the original position. */
	fseek( fp, save_pos, SEEK_SET );

	return( size_of_file );
}

void CLog::CheckFile()
{
	if ( GetFileSize(m_fp) > MAX_LOG_FILE_SIZE )
	{
		fclose(m_fp);
		m_fp = NULL;
		++m_nFileIndex;
		if ( m_nFileIndex >= MAX_FILE_LOG_COUNT )
		{
			m_nFileIndex = 0;
		}
		m_fp = GetFilePointer(m_szFilePath);
	}
}

void CLog::WriteLogEx(const int outTarget,const int level,const char* str,...)
{
	char szInfo[MAX_PATH_LENGTH]={0};
	va_list arg_ptr;
	va_start(arg_ptr,str);
	vsnprintf(szInfo,sizeof(szInfo)-1,str,arg_ptr);

	WriteLog(outTarget,level,szInfo);

}
void CLog::WriteLog(const int outTarget,const int level,const char* str)
{
	if (level > m_nLevel)
	{
		return;
	}
	char szLog[MAX_PATH_LENGTH]={0};
#if defined(WIN32)
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(szLog,"%s_%d/%02d/%02d_%02d:%02d:%02d:%03d : %s",LOG_LEVEL_CONTENT[level],st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,str);
#else
	char cur_time[20] = {0};
	struct timeval st;
	gettimeofday(&st,NULL);
	strftime(cur_time,sizeof(szLog),"%Y/%m/%d %T",localtime(&st.tv_sec));
	sprintf(szLog,"%s_%s : %s",LOG_LEVEL_CONTENT[level],cur_time,str);
#endif
	int nLen = strlen(szLog);
	pthread_mutex_lock(&m_lock);
	if ( outTarget&LOG_FILE_OUT && m_pFileBuffer+m_nFileBufferWritePos+nLen < m_pFileBuffer+LOG_FILE_BUFFER_SIZE)
	{
		memcpy(m_pFileBuffer+m_nFileBufferWritePos,szLog,nLen);
		m_nFileBufferWritePos += nLen;
	}
	if (outTarget&LOG_STD_OUT && m_pStdBuffer+m_nStdBufferWritePos+nLen < m_pStdBuffer+LOG_STD_BUFFER_SIZE)
	{
		memcpy(m_pStdBuffer+m_nStdBufferWritePos,szLog,nLen);
		m_nStdBufferWritePos += nLen;
	}
	pthread_mutex_unlock(&m_lock);
}
void* CLog::ThreadStdWriteProc(void* param)
{
	CLog* pLog = (CLog*)param;
	int nLogSize = 0;
	char* pBuf = (char*)malloc(LOG_STD_BUFFER_SIZE);
	memset(pBuf,0, LOG_FILE_BUFFER_SIZE);
	
	while(!pLog->m_bStopLog)
	{
		pthread_mutex_lock(&pLog->m_lock);
		memcpy(pBuf,pLog->m_pStdBuffer,pLog->m_nStdBufferWritePos);
		nLogSize = pLog->m_nStdBufferWritePos;
		pLog->m_nStdBufferWritePos = 0;
		pthread_mutex_unlock(&pLog->m_lock);

		if (nLogSize>0)
		{
			fwrite(pBuf, 1, nLogSize, stdout);
		}
		usleep(1000 * 1000);
	}
	free(pBuf);
	return NULL;
}
void* CLog::ThreadFileWriteProc(void* param)
{
	CLog* pLog = (CLog*)param;
	int nLogSize = 0;
	char* pBuf = (char*)malloc(LOG_FILE_BUFFER_SIZE);
	memset(pBuf,0, LOG_FILE_BUFFER_SIZE);

	while(!pLog->m_bStopLog)
	{
		memset(pBuf,0, LOG_FILE_BUFFER_SIZE);
		pthread_mutex_lock(&pLog->m_lock);
		memcpy(pBuf,pLog->m_pFileBuffer,pLog->m_nFileBufferWritePos);
		nLogSize = pLog->m_nFileBufferWritePos;
		pLog->m_nFileBufferWritePos = 0;
		pthread_mutex_unlock(&pLog->m_lock);
		if (NULL == pLog->m_fp)
		{
			break;
		}
		if (nLogSize>0)
		{
			if ( pLog->m_bFileFirstLine && pLog->GetFileSize(pLog->m_fp) > 0 )
			{
				fwrite("\n\n\n",1,3,pLog->m_fp);
			}
			pLog->m_bFileFirstLine = false;
			fwrite(pBuf, 1, nLogSize, pLog->m_fp);
		}
		fflush(pLog->m_fp);
		pLog->CheckFile();
		usleep(1000 * 1000);
	}
	free(pBuf);
	return NULL;
}
void CLog::UnInit()
{
	m_bStopLog = true;
	pthread_join(m_threadIDStd,NULL);
	pthread_join(m_threadIDFile,NULL);
	if (NULL != m_fp)
	{
		fclose(m_fp);
		m_fp = NULL;
	}
}
void log_printf(const int level,const char* str,...)
{
	char szInfo[MAX_PATH_LENGTH]={0};
	va_list arg_ptr;
	va_start(arg_ptr,str);
	vsnprintf(szInfo,sizeof(szInfo)-1,str,arg_ptr);

	CLog::GetIntance().WriteLog(LOG_ALL_OUT,level,szInfo);
}
void log_init(const char* sPath)
{
	CLog::GetIntance().Init(sPath);
}