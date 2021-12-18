#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winbase.h>
#include <math.h>
#include <time.h>


#define TIMEOUT 1000

uint8_t convert (char* buf, float *temp, float *pres, float *hum);


int main()
{
	HANDLE port = CreateFile("\\\\.\\COM5", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (port == INVALID_HANDLE_VALUE)
	{
		printf("Error. Invalid handle value");
		system("pause");
		return 0;
	}

	DCB dcb = {0};

	dcb.DCBlength			= sizeof(DCB);
	GetCommState(port, &dcb);
	dcb.BaudRate			= CBR_9600;
	dcb.ByteSize			= 8;
	dcb.Parity				= NOPARITY;
	dcb.StopBits			= ONESTOPBIT;
	dcb.fAbortOnError		= TRUE;
	dcb.fDtrControl			= DTR_CONTROL_DISABLE;
	dcb.fRtsControl			= RTS_CONTROL_DISABLE;
	dcb.fBinary				= TRUE;
	dcb.fParity				= FALSE;
	dcb.fInX				= FALSE;
	dcb.fOutX				= FALSE;
	dcb.XonChar				= 0;
	dcb.XoffChar			= 0xFF;
	dcb.fErrorChar			= FALSE;
	dcb.fNull				= FALSE;
	dcb.fOutxCtsFlow		= FALSE;
	dcb.fOutxDsrFlow		= FALSE;
	dcb.XonLim				= 128;
	dcb.XoffLim				= 128;

	if ( !SetCommState (port, &dcb))
	{
		printf("DCB out Invalid\n");
		system("pause");
		return 0;
	}

	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = TIMEOUT;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = TIMEOUT;

	if ( !SetCommTimeouts(port, &CommTimeOuts) )
	{
		printf("Time out Invalid\n");
		system("pause");
		return 0;
	}

	char buf[255] = {0};

	SYSTEMTIME st;
    GetLocalTime(&st);

	char file_name[255] = "log/";
	char mas[5] = {0};

	itoa(st.wYear, mas, 10);
	strcat(file_name, mas);
	strcat(file_name, "-");

	itoa(st.wMonth, mas, 10);
	strcat(file_name, mas);
	strcat(file_name, "-");

	itoa(st.wDay, mas, 10);
	strcat(file_name, mas);
	strcat(file_name, " ");

	itoa(st.wHour, mas, 10);
	strcat(file_name, mas);
	strcat(file_name, "-");

	itoa(st.wMinute, mas, 10);
	strcat(file_name, mas);
	strcat(file_name, "-");

	itoa(st.wSecond, mas, 10);
	strcat(file_name, mas);
	strcat(file_name, "-");

	itoa(st.wMilliseconds, mas, 10);
	strcat(file_name, mas);

	strcat(file_name, ".csv");

	FILE *file = fopen(file_name, "a");

	fprintf(file, "date;time;milliseconds;temperature;pressure;humidity;\n");
	fclose(file);

	unsigned long size	= 0;
	float temp = 0;
	float pres = 0;
	float hum = 0;
	char start = '1';


	while (1)
	{
		if ( !WriteFile(port, &start, sizeof(start), &size, 0) )
			printf("Alalrm! Can't write!\n");
		if ( ReadFile(port, buf, sizeof(buf), &size, 0) )
		{
			if ( convert (buf, &temp, &pres, &hum) )
			{
				printf("%s\n", buf);
				printf("Error\n");
				continue;
			}
			printf("temp = %0.3f, pres = %0.3f, hum = %0.3f\n", temp, pres, hum);
			FILE *file = fopen(file_name, "a");
			GetLocalTime(&st);

			fprintf(file, "%2d-%2d-%2d;%2d:%2d:%2d;%3d;%0.3f;%0.3f;%0.3f;\n",	st.wYear, st.wMonth,
																				st.wDay, st.wHour,
																				st.wMinute, st.wSecond,
																				st.wMilliseconds, temp,
																				pres, hum);
			fclose(file);
		}
	}

	if ( CloseHandle(port) )
		printf("Good job\n");

	system("pause");

	return 0;
}

uint8_t convert (char* buf, float *temp, float *pres, float *hum)
{
	uint8_t count = 0;
	uint8_t b = 0;
	int32_t tmp = 0;
	uint8_t neg = 0;

	if (buf[count] != 't')
		return 1;		//Не температура

	count+=2;

	while (buf[count] != ';')
		count++;

	count--;

	while (buf[count] != ':')
	{
		if (buf[count] == '-')
		{
			neg = 1;		//Пришло отрицательное значение
			break;
		}

		tmp += (buf[count--] - '0')*pow(10, b++);
	}
	count += b+2;
	b = 0;

	if (neg)
	{
		tmp = 0 - tmp;
		neg = 0;
	}

	tmp = (tmp * 5 + 128) >> 8;
	*temp = (float)tmp/100;
	tmp = 0;


//------------------
	if (buf[count] != 'p')
		return 2;		//Не давление

	count+=2;

	while (buf[count] != ';')
		count++;

	count--;

	while (buf[count] != ':')
	{
		if (buf[count] == '-')
		{
			neg = 1;		//Пришло отрицательное значение
			break;
		}

		tmp += (buf[count--] - '0')*pow(10, b++);
	}
	count += b+2;
	b = 0;

	if (neg)
	{
		tmp = 0 - tmp;
		neg = 0;
	}

	*pres = (float)tmp/256;
	tmp = 0;


//------------------
	if (buf[count] != 'h')
		return 2;		//Не влажность

	count+=2;

	while (buf[count] != ';')
		count++;

	count--;

	while (buf[count] != ':')
	{
		if (buf[count] == '-')
		{
			neg = 1;		//Пришло отрицательное значение
			break;
		}

		tmp += (buf[count--] - '0')*pow(10, b++);
	}

	if (neg)
	{
		tmp = 0 - tmp;
		neg = 0;
	}

	*hum = (float)tmp/1024;

	return 0;
}