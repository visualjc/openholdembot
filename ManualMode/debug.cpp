//******************************************************************************
//
// This file is part of the OpenHoldem project
//    Source code:           https://github.com/OpenHoldem/openholdembot/
//    Forums:                http://www.maxinmontreal.com/forums/index.php
//    Licensed under GPL v3: http://www.gnu.org/licenses/gpl.html
//
//******************************************************************************
//
// Purpose:
//
//******************************************************************************


#include "stdafx.h"
#include "debug.h"
#include "ManualMode.h"
#include "ManualModeDlg.h"
#include "..\..\dbghelp\dbghelp.h"

FILE *log_fp = NULL;

char * get_time(char * timebuf) 
{
	// returns current system time in WH format
	time_t	ltime;
	char tmptime[30];
	
	time( &ltime );
	ctime_s(tmptime, 26, &ltime);
	tmptime[24]='\0';

	memcpy(timebuf, tmptime+20, 4); //yyyy
	*(timebuf+4) = '-';

	// mm
	if (memcmp(tmptime+4, "Jan", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '1'; }
	else if (memcmp(tmptime+4, "Feb", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '2'; }
	else if (memcmp(tmptime+4, "Mar", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '3'; }
	else if (memcmp(tmptime+4, "Apr", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '4'; }
	else if (memcmp(tmptime+4, "May", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '5'; }
	else if (memcmp(tmptime+4, "Jun", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '6'; }
	else if (memcmp(tmptime+4, "Jul", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '7'; }
	else if (memcmp(tmptime+4, "Aug", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '8'; }
	else if (memcmp(tmptime+4, "Sep", 3)==0)  { *(timebuf+5) = '0'; *(timebuf+6) = '9'; }
	else if (memcmp(tmptime+4, "Oct", 3)==0)  { *(timebuf+5) = '1'; *(timebuf+6) = '0'; }
	else if (memcmp(tmptime+4, "Nov", 3)==0)  { *(timebuf+5) = '1'; *(timebuf+6) = '1'; }
	else if (memcmp(tmptime+4, "Dec", 3)==0)  { *(timebuf+5) = '1'; *(timebuf+6) = '2'; }

	*(timebuf+7) = '-';
	memcpy(timebuf+8, tmptime+8, 2); //dd
	*(timebuf+10) = ' ';
	memcpy(timebuf+11, tmptime+11, 8); //HH:mm:ss
	*(timebuf+19) = '\0';

	return timebuf;
}

char * get_now_time(char * timebuf) 
{
	// returns current system time as a UNIX style string
	time_t	ltime;

	time( &ltime );
	ctime_s(timebuf, 26, &ltime);
	timebuf[24]='\0';
	return timebuf;
}

void logfatal (char* fmt, ...) 
{
	char		buff[10000] ;
	va_list		ap;
	char		fatallogpath[MAX_PATH];
	FILE		*fatallog;
	char		nowtime[26];

	sprintf_s(fatallogpath, MAX_PATH, "%s\\fatal error.log", startup_path);
	if (fopen_s(&fatallog, fatallogpath, "a")==0)
	{
		va_start(ap, fmt);
		vsprintf_s(buff, 10000, fmt, ap);
		fprintf(fatallog, "%s> %s", get_now_time(nowtime), buff);
		
		va_end(ap);
		fclose(fatallog);
	}
}

BOOL CreateBMPFile(const char *szFile, HBITMAP hBMP) 
{
	// Saves the hBitmap as a bitmap.
	HDC					hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL); 
	HDC					hdcCompatible = CreateCompatibleDC(hdcScreen); 
	PBITMAPINFO			pbmi=NULL;
	BOOL				bret=FALSE;
	HANDLE				hf=NULL; // file handle
	BITMAPFILEHEADER	hdr; // bitmap file-header
	PBITMAPINFOHEADER	pbih=NULL; // bitmap info-header
	LPBYTE				lpBits=NULL; // memory pointer
	DWORD				dwTotal=0; // total count of bytes
	DWORD				cb=0; // incremental count of bytes
	BYTE				*hp=NULL; // byte pointer
	DWORD				dwTmp=0;
	BITMAP				bmp;

	memset(&bmp,0,sizeof(BITMAP));
	GetObject(hBMP,sizeof(BITMAP),&bmp);
	memset(&hdr,0,sizeof(hdr));
	{
		int cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
		if (cClrBits>8) {
			// No Palette (normally)
			pbmi = (PBITMAPINFO) calloc(1, sizeof(BITMAPINFOHEADER));
		}
		else {
			pbmi = (PBITMAPINFO) calloc(1, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<<(min(8,cClrBits))));
			pbmi->bmiHeader.biClrUsed = (1<<cClrBits);
		}

		// Initialize the fields in the BITMAPINFO structure.
		pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

		// Retrieve the color table (RGBQUAD array) and the bits
		// (array of palette indices) from the DIB.
		if (!GetDIBits(hdcCompatible, hBMP, 0, bmp.bmHeight, NULL, pbmi, DIB_RGB_COLORS)) {
			goto to_return;
		}
	}
	pbih = &(pbmi->bmiHeader);
	pbmi->bmiHeader.biCompression=BI_RGB;
	lpBits = (LPBYTE) calloc(1, pbih->biSizeImage);

	if (!lpBits) { goto to_return; }

	if (!GetDIBits(hdcCompatible, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbmi, DIB_RGB_COLORS)) {
		goto to_return;
	}

	// Create the .BMP file.
	hf = CreateFile(szFile, GENERIC_READ | GENERIC_WRITE, (DWORD) 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		goto to_return;
	}
	hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M"
	// Compute the size of the entire file.
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed*sizeof(RGBQUAD) + pbih->biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;

	// Compute the offset to the array of color indices.
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed*sizeof (RGBQUAD);

	// Copy the BITMAPFILEHEADER into the .BMP file.
	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER),	(LPDWORD) &dwTmp, NULL)) {
		goto to_return;
	}

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
	if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof (RGBQUAD), (LPDWORD) &dwTmp, ( NULL))) {
		goto to_return;
	}

	// Copy the array of color indices into the .BMP file.
	dwTotal = cb = pbih->biSizeImage;
	hp = lpBits;
	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) { goto to_return; }

	// Close the .BMP file.
	if (!CloseHandle(hf)) { goto to_return; }
	bret=TRUE;


	to_return:;
	// Free memory.
	if(pbmi)free(pbmi);
	if(lpBits)free(lpBits);
	DeleteDC(hdcCompatible);
	DeleteDC(hdcScreen);

	return bret;
}