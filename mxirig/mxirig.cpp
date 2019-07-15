/*
  Copyright (C) MOXA Inc. All rights reserved.
  This software is distributed under the terms of the
  MOXA License.  See the file COPYING-MOXA for details.
*/

/**
 * @file mxirig.cpp : interface of the Moxa IRIGB Card.
 *
 * @version 1.2.0.0 - 2017/07/01
 *
 * @author holsety.chen@moxa.com
 */

#ifdef WIN32
#include <Windows.h>
#include <WinIoCtl.h>
#endif

#include <stdio.h>
#include <time.h>
#include "Public.h"
#include "RegmxIrigbPci.h"
#include "mxirig.h"

#ifdef WIN32
extern HANDLE _stdcall InitializeMxDrv(int devindex);
extern void _stdcall ShutdownMxDrv(HANDLE hDevice);
#endif

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

/**
 * Set/Clear register bits value to FPGA
 * @param  [in] hDev - the handle value return from "mxIrigbOpen" function
 * @param  [in] address - register address.
 * @param  [in] setbits - set bits.
 * @param  [in] clrbits - clear bits.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxirigb_setclrreg(HANDLE hDev, DWORD address, DWORD setbits, DWORD clrbits)
{
#ifdef WIN32
	DWORD pdwAddress[3];
	DWORD dwBytesReturned;

	pdwAddress[0] = address;
	pdwAddress[1] = setbits;
	pdwAddress[2] = clrbits;

	return DeviceIoControl(hDev, IOCTL_SETCLR_REGISTER_BIT, pdwAddress,
		sizeof(pdwAddress), NULL, 0, &dwBytesReturned, NULL);
#else
	struct reg_bit_pair_struct set;

	set.addr = address;
	set.set_bit = setbits;
	set.clear_bit = clrbits;

	return (ioctl(hDev, IOCTL_SETCLR_REGISTER_BIT, &set) == 0 ) ? TRUE : FALSE;
#endif
}

/**
 * Set register value to FPGA
 * @param  [in] hDev - the handle value return from "mxIrigbOpen" function
 * @param  [in] address - register address.
 * @param  [in] value - register data.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxirigb_setreg(HANDLE hDev, DWORD address, DWORD value)
{
	DWORD dwBytesReturned;

#ifdef WIN32
	DWORD dwWrite[2];
	dwWrite[0] = address;
	dwWrite[1] = value;

	return DeviceIoControl(hDev, IOCTL_SET_REGISTER, dwWrite,
		sizeof(dwWrite), NULL, 0, &dwBytesReturned, NULL);
#else
	struct reg_val_pair_struct set;

	set.count = 1;
	set.addr[0] = address;
	set.val[0] = value;

	return (ioctl(hDev, IOCTL_SET_REGISTER, &set) == 0 ) ? TRUE : FALSE;
#endif
}

/**
 * Get register value from FPGA
 * @param  [in] hDev - the handle value return from "mxIrigbOpen" function
 * @param  [in] address - register address.
 * @param  [out] pValue - register data.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxirigb_getreg(HANDLE hDev, DWORD address, PDWORD pValue)
{
#ifdef WIN32
	DWORD dwBytesReturned;

	return DeviceIoControl(hDev, IOCTL_GET_REGISTER, &address,
		sizeof(address), pValue, sizeof(DWORD), &dwBytesReturned, NULL);
#else
	struct reg_val_pair_struct get;
	int retval;

	get.count = 1;
	get.addr[0]=address;
	retval = ioctl(hDev, IOCTL_GET_REGISTER, &get);
    *pValue = get.val[0];

	/* In Linux system, the return ( value == 0 ) means TRUE */
	return (retval == 0) ? TRUE : FALSE;
#endif
}

/**
 * Get Irigb board hardware ID
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwHwId - A pointer to get hardware ID, the value is one of _IRIGB_BOARD_HWID_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetHardwareID(HANDLE hDev, PDWORD pdwHwId)
{
	DWORD dwValue;
	BOOL bRet = mxirigb_getreg(hDev, PORTDAT, &dwValue);

	if (bRet) {
		// The hardware id pin is GPI13~15
		*pdwHwId = ((dwValue >> PORTDATA_INPUT_BIT_S) & PORTDATA_MASK) >> 13;
	}

	return bRet;
}

/**
 * Open Irigb device
 * @param  [in] index - the device number (started from 0)
 * @return Pointer to device handle. Return -1 on failure.
 */
MXIRIG_API HANDLE mxIrigbOpen(int index)
{
	DWORD dwHwId;

#ifdef WIN32
	HANDLE hDev = InitializeMxDrv(index);

	if (hDev == INVALID_HANDLE_VALUE || hDev == NULL) {
		return (HANDLE) -1;
	}

#else
	HANDLE hDev=open("/dev/moxa_irigb", O_RDWR);
	if ( hDev < 0 ) {
		printf("open /dev/moxa_irigb fail\n");
		return -1;
	}
#endif

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		mxIrigbClose(hDev);
		return (HANDLE) -1;
	}

	/* Enable IRIG-B input module */
	mxirigb_setclrreg( hDev, INPORTCON, 
		INPORTCON_BIT_IRIGDE0_DIS | INPORTCON_BIT_IRIGDE1_DIS, 0 );

	if (dwHwId == DA_IRIGB_S) {
		/* Configure Output LED */
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_OUTP1 << NLED_P2_BIT_S),
			(NLED_MODE_MASK << NLED_P2_BIT_S) );
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_OUTP3 << NLED_P4_BIT_S),
			(NLED_MODE_MASK << NLED_P4_BIT_S) );
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_OUTP2 << NLED_P1_BIT_S),
			(NLED_MODE_MASK << NLED_P1_BIT_S) );
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_OUTP4 << NLED_P3_BIT_S),
			(NLED_MODE_MASK << NLED_P3_BIT_S) );

		/* Configure Input LED */
		/* --> Configure by Time source select function */
	} else if ((dwHwId == DA_IRIGB_4DIO_PCI104) ||
				(dwHwId == DE2_IRIGB_4DIO)) {
		/* Configure Output LED */
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_OUTP1 << NLED_P3_BIT_S),
			(NLED_MODE_MASK << NLED_P3_BIT_S) );
		/* Configure Input LED */
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_INP1 << NLED_P1_BIT_S),
			(NLED_MODE_MASK << NLED_P1_BIT_S) );
		mxirigb_setclrreg(hDev, NLEDCON, 
			(NLED_MODE_INP2 << NLED_P2_BIT_S),
			(NLED_MODE_MASK << NLED_P2_BIT_S) );
	}

	return hDev;
}

/**
 * Close Irigb device
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @return None
 */
MXIRIG_API void mxIrigbClose(HANDLE hDev)
{

	#ifdef WIN32
	if (hDev == INVALID_HANDLE_VALUE || hDev == NULL) {
		return ;
	}
		ShutdownMxDrv(hDev);
	#else
		close(hDev);
	#endif
}

/**
 * Get internal RTC time from Irigb device
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pRtcTime - A pointer to a RTCTIME structure to receive the current date and time.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetTime(HANDLE hDev, PRTCTIME pRtcTime)
{
	DWORD pdwAddress[4] = { RTCDAT0, RTCDAT1, RTCDAT2, RTCDAT3 };
	DWORD pdwValue[4];
	DWORD dwBytesReturned;
	BOOL bRet;
#ifdef WIN32
	bRet = DeviceIoControl(hDev, IOCTL_GET_REGISTER,
		pdwAddress, sizeof(pdwAddress), pdwValue, sizeof(pdwValue),
		&dwBytesReturned, NULL);
#else
	struct reg_val_pair_struct get;
	int i;

	memset(&get, 0, sizeof(get));
	get.count = 4;
	for ( i=0; i<get.count; i++ ) {
		get.addr[i] = pdwAddress[i];
	}

	bRet = ioctl(hDev, IOCTL_GET_REGISTER, &get);
	/* In Linux system, the return ( value == 0 ) means TRUE */
	bRet = ( bRet == 0 ) ? TRUE : FALSE;

	for ( i=0; i<get.count; i++ ) {
		pdwValue[i] = get.val[i];
	}
#endif
	if (!bRet) {
		return bRet;
	}

	/* Transfer BCD to HEX */
	/* RTCDAT0[7:0] */
	pRtcTime->sec = (pdwValue[0]&0xf) + ((pdwValue[0]>>4)&0xf)*10;
	/* RTCDAT0[15:8] */
	pRtcTime->min = ((pdwValue[0]>>8)&0xf) + ((pdwValue[0]>>12)&0xf)*10;
	/* RTCDAT0[23:16] */
	pRtcTime->hour = ((pdwValue[0]>>16)&0xf) + ((pdwValue[0]>>20)&0xf)*10;
	/* RTCDAT0[31:24] */
	pRtcTime->mday = ((pdwValue[0]>>24)&0xf) + ((pdwValue[0]>>28)&0xf)*10;
	/* RTCDAT1[7:0] */
	pRtcTime->mon = (pdwValue[1]&0xf) + ((pdwValue[1]>>4)&0xf)*10;
	/* RTCDAT1[23:8] */
	pRtcTime->year = ((pdwValue[1]>>8)&0xf) + ((pdwValue[1]>>12)&0xf)*10 +
		((pdwValue[1]>>16)&0xf)*100 + ((pdwValue[1]>>20)&0xf)*1000;
	/* RTCDAT2[31:0] HEX */
	pRtcTime->nanosec = pdwValue[2];

	pRtcTime->lsp = (pdwValue[3] & RTCDAT3_BIT_LSP) ? 1:0;
	pRtcTime->ls = (pdwValue[3] & RTCDAT3_BIT_LS) ? 1:0;
	pRtcTime->dsp = (pdwValue[3] & RTCDAT3_BIT_DSP) ? 1:0;
	pRtcTime->dst = (pdwValue[3] & RTCDAT3_BIT_DST) ? 1:0;
	pRtcTime->tzs = (pdwValue[3] & RTCDAT3_BIT_TZS) ? 1:0;
	pRtcTime->tzh = (pdwValue[3] & RTCDAT3_BIT_TZH) ? 1:0;
	pRtcTime->tz = (pdwValue[3] >> RTCDAT3_TZ_BIT_S) & RTCDAT3_TZ_MASK;
	pRtcTime->tq = (pdwValue[3] >> RTCDAT3_TQ_BIT_S) & RTCDAT3_TQ_MASK;

	/* WORKAROUND: avoid FPGA leap second issue */
	if ( pRtcTime->lsp ) {
		time_t rawtime;
		struct tm *systime;

		if ( pRtcTime->ls ) { /* -1 */
			if ( pRtcTime->sec == 59 ) {
				/* ex: 07:59:59 -> 08:00:00 */
				time (&rawtime);
				systime = localtime (&rawtime);

				systime->tm_year = pRtcTime->year - 1900;
				systime->tm_mon  = pRtcTime->mon - 1;
				systime->tm_mday = pRtcTime->mday;
				systime->tm_hour = pRtcTime->hour;
				systime->tm_min = pRtcTime->min;
				systime->tm_sec = pRtcTime->sec;
				rawtime = mktime(systime);
				rawtime++;

				systime = localtime (&rawtime);

				pRtcTime->year = systime->tm_year + 1900;
				pRtcTime->mon  = systime->tm_mon + 1;
				pRtcTime->mday = systime->tm_mday;
				pRtcTime->hour = systime->tm_hour;
				pRtcTime->min  = systime->tm_min;
				pRtcTime->sec  = systime->tm_sec;

				pRtcTime->ls = 0;
				pRtcTime->lsp = 0;
			}
		} else {
			/* +1 */
			if ( pRtcTime->sec == 0 ) {
				/* ex: 08:00:00 -> 07:59:60 */
				time (&rawtime);
				systime = localtime (&rawtime);

				systime->tm_year = pRtcTime->year - 1900;
				systime->tm_mon  = pRtcTime->mon - 1;
				systime->tm_mday = pRtcTime->mday;
				systime->tm_hour = pRtcTime->hour;
				systime->tm_min = pRtcTime->min;
				systime->tm_sec = pRtcTime->sec;
				rawtime = mktime(systime);
				rawtime--;

				systime = localtime (&rawtime);

				pRtcTime->year = systime->tm_year + 1900;
				pRtcTime->mon  = systime->tm_mon + 1;
				pRtcTime->mday = systime->tm_mday;
				pRtcTime->hour = systime->tm_hour;
				pRtcTime->min  = systime->tm_min;
				pRtcTime->sec  = systime->tm_sec + 1;
			} else if ( pRtcTime->sec == 61 ) {
				/* ex: 07:59:61 -> 08:00:00 */
				time (&rawtime);
				systime = localtime (&rawtime);

				systime->tm_year = pRtcTime->year - 1900;
				systime->tm_mon  = pRtcTime->mon - 1;
				systime->tm_mday = pRtcTime->mday;
				systime->tm_hour = pRtcTime->hour;
				systime->tm_min = pRtcTime->min;
				systime->tm_sec = 59;
				rawtime = mktime(systime);
				rawtime++;

				systime = localtime (&rawtime);

				pRtcTime->year = systime->tm_year + 1900;
				pRtcTime->mon  = systime->tm_mon + 1;
				pRtcTime->mday = systime->tm_mday;
				pRtcTime->hour = systime->tm_hour;
				pRtcTime->min  = systime->tm_min;
				pRtcTime->sec  = systime->tm_sec;

				pRtcTime->lsp = 0;
			}
		}
	}

	return bRet;
}

/**
 * Set internal RTC time to Irigb device
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] pRtcTime - A pointer to a RTCTIME structure that contains the new date and time.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetTime(HANDLE hDev, PRTCTIME pRtcTime)
{
	int monthTable[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	DWORD pdwAddress[4];
	DWORD dwBytesReturned;
	DWORD dwSyncTimeSource;
	BOOL ret = FALSE;

	/* Check out of range */
	if (pRtcTime->sec<0 || pRtcTime->sec>59 ||
		pRtcTime->min<0 || pRtcTime->min>59 ||
		pRtcTime->hour<0 || pRtcTime->hour>23 ||
		pRtcTime->mday<1 || pRtcTime->mday>31 ||
		pRtcTime->mon<1 || pRtcTime->mon>12 ||
		pRtcTime->year<0 || pRtcTime->year>9999) {
		return FALSE;
	}

	/* check leap year */
	if (((pRtcTime->year%400)==0) ||
		((pRtcTime->year%4)==0) && ((pRtcTime->year%100)!=0)) {
		monthTable[1]++;
	}

	/* check the month day range */
	if (pRtcTime->mday > monthTable[pRtcTime->mon-1]) {
		return FALSE;
	}

	/* Before set time to internal RTC,
	 * must change Sync. time source to "Free run".
	 */
	if (!mxIrigbGetSyncTimeSrc( hDev, &dwSyncTimeSource )) {
		return FALSE;
	}

	mxIrigbSetSyncTimeSrc( hDev, TIMESRC_FREERUN );

	pdwAddress[0] = RTCDAT0;
	pdwAddress[1] = 
		(pRtcTime->sec%10 + ((pRtcTime->sec/10)<<4)) |          // RTCDAT0[7:0]
		(pRtcTime->min%10 + ((pRtcTime->min/10)<<4)) << 8 |     // RTCDAT0[15:8]
		(pRtcTime->hour%10 + ((pRtcTime->hour/10)<<4)) << 16 |  // RTCDAT0[23:16]
		(pRtcTime->mday%10 + ((pRtcTime->mday/10)<<4)) << 24 ;  // RTCDAT0[31:24]
	pdwAddress[2] = RTCDAT1;
	pdwAddress[3] = RTCDAT1_BIT_COMMIT_TIME |                   // RTCDAT1[31], commit time value
		((pRtcTime->mon%10) + ((pRtcTime->mon/10)<<4)) |        // RTCDAT1[7:0]
		((pRtcTime->year%10) << 8) |                            // RTCDAT1[11:8]
		(((pRtcTime->year/10)%10) << 12) |                      // RTCDAT1[15:12]
		(((pRtcTime->year/100)%10) << 16) |                     // RTCDAT1[19:16]
		(((pRtcTime->year/1000)%10) << 20) ;                    // RTCDAT1[23:20]

#ifdef WIN32
	ret = DeviceIoControl(hDev, IOCTL_SET_REGISTER, pdwAddress,
		sizeof(pdwAddress), NULL, 0, &dwBytesReturned, NULL);
#else
	struct reg_val_pair_struct set;
	int i;

	memset(&set, 0, sizeof(set));
	set.count = 2;
	for ( i=0; i < set.count; i++ ) {
		set.addr[i] = pdwAddress[i*2];
		set.val[i] = pdwAddress[(i*2)+1];
	}

	/* In Linux system, the return ( value == 0 ) means TRUE */
	ret = ( ioctl(hDev, IOCTL_SET_REGISTER, &set) == 0 ) ? TRUE : FALSE;
#endif

	mxIrigbSetSyncTimeSrc( hDev, dwSyncTimeSource );

	return ret;
}

/**
 * Synchronize local time with internal RTC
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] pRtbToFrom - 0: Set internal RTC to local time, 1: Set local time to internal RTC
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSyncTime(HANDLE hDev, BOOL bToFrom)
{
	BOOL bRet = FALSE;
#ifdef WIN32
	SYSTEMTIME systime;
#else
	time_t rawtime;
	struct tm *systime;
#endif
	RTCTIME rtctime;

	if (bToFrom) {
#ifdef WIN32
		GetLocalTime(&systime);
		if (systime.wMilliseconds>0) {
			Sleep(1000-systime.wMilliseconds);
		}
		GetLocalTime(&systime);
		rtctime.year = systime.wYear;
		rtctime.mon = systime.wMonth;
		rtctime.mday = systime.wDay;
		rtctime.hour = systime.wHour;
		rtctime.min = systime.wMinute;
		rtctime.sec = systime.wSecond;
#else
        struct timespec spec;
        long ms;

		clock_gettime(CLOCK_REALTIME, &spec);
		ms = spec.tv_nsec / 1000000; // Milliseconds
		if ( ms > 0 ) {
			usleep(1000 - ms);
		}

		time (&rawtime);
		systime = localtime (&rawtime);

		rtctime.year = systime->tm_year + 1900;
		rtctime.mon  = systime->tm_mon + 1;
		rtctime.mday = systime->tm_mday;
		rtctime.hour = systime->tm_hour;
		rtctime.min  = systime->tm_min;
		rtctime.sec  = systime->tm_sec;
#endif
		bRet = mxIrigbSetTime(hDev, &rtctime);
	} else {
		bRet = mxIrigbGetTime(hDev, &rtctime);
#ifdef WIN32
		systime.wYear = rtctime.year;
		systime.wMonth = rtctime.mon;
		systime.wDay = rtctime.mday;
		systime.wHour = rtctime.hour;
		systime.wMinute = rtctime.min;
		if (rtctime.sec == 60 ) {
			rtctime.sec = 59;
		}
		systime.wSecond = rtctime.sec;
		systime.wMilliseconds = rtctime.nanosec / 1000000;
		SetLocalTime(&systime);
#else
		/* Jared, first get the system time. */
		time (&rawtime);
		systime = localtime (&rawtime);

		/* Then sync the time from IRIG-B RTC */
		systime->tm_year = rtctime.year - 1900;
		systime->tm_mon  = rtctime.mon - 1;
		systime->tm_mday = rtctime.mday;
		systime->tm_hour = rtctime.hour;
		systime->tm_min = rtctime.min;
		/* In Linux, normally in the range 0 to 59, but can be up to 60 to allow for leap seconds. */
		systime->tm_sec = rtctime.sec;
		/* The Linux system time structure, struct tm, doesn't has miniseconds information.
		 * So we do some delay here.
		 */
		/* After the small delay, sync the time to system clock. */
		rawtime = mktime(systime);
		if ( stime(&rawtime) < 0 ) {
			printf("stime() fail\n");
			bRet = FALSE;
		}
#endif
	}

	return bRet;
}

/**
 * Set internal RTC synchronization source
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - internal RTC synchronization source, the value is one of _RTC_SYNC_SOURCE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetSyncTimeSrc(HANDLE hDev, DWORD dwSource)
{
	DWORD dwHwId;

	if( dwSource >= TIMESRC_UNKNOWN) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	if (dwHwId == DA_IRIGB_S) {
		/* Configure Sync. LED */
		if (dwSource==TIMESRC_FIBER) {
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_IRIG0_OK << NLED_P0_BIT_S),
				(NLED_MODE_MASK << NLED_P0_BIT_S) );

		/* Configure RX LED */
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_INP0 << NLED_P5_BIT_S),
				(NLED_MODE_MASK << NLED_P5_BIT_S) );
		} else if (dwSource==TIMESRC_PORT1) {
			DWORD dwType;
			BOOL pbInvert;
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_IRIG1_OK << NLED_P0_BIT_S),
				(NLED_MODE_MASK << NLED_P0_BIT_S) );
			if (mxIrigbGetInputSignalType(hDev,
				PORT_1, &dwType, &pbInvert)) {
				if (dwType==TYPE_TTL) {
					mxirigb_setclrreg(hDev, NLEDCON, 
						(NLED_MODE_INP2 << NLED_P5_BIT_S),
						(NLED_MODE_MASK << NLED_P5_BIT_S) );
				} else if (dwType==TYPE_DIFFERENTIAL) {
					mxirigb_setclrreg(hDev, NLEDCON, 
						(NLED_MODE_INP1 << NLED_P5_BIT_S),
						(NLED_MODE_MASK << NLED_P5_BIT_S) );
				}
			}
		}
	} else if (dwHwId == DE2_IRIGB_4DIO) {
		/* Configure Sync. LED */
		if (dwSource==TIMESRC_FIBER) {
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_IRIG0_OK << NLED_P0_BIT_S),
				(NLED_MODE_MASK << NLED_P0_BIT_S) );
		} else if (dwSource==TIMESRC_PORT1) {
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_IRIG1_OK << NLED_P0_BIT_S),
				(NLED_MODE_MASK << NLED_P0_BIT_S) );
		}
	}

	return mxirigb_setclrreg(hDev, RTCCON, dwSource, RTCCON_SYNCSRC_MASK);
}

/**
 * Get internal RTC synchronization source
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwSource - A point to get internal RTC synchronization source, the value is one of _RTC_SYNC_SOURCE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetSyncTimeSrc(HANDLE hDev, PDWORD pdwSource)
{
	DWORD dwValue;
	if (!mxirigb_getreg(hDev, RTCCON, &dwValue)) {
		return FALSE;
	}
	
	dwValue &= RTCCON_SYNCSRC_MASK;
	if (dwValue>=TIMESRC_UNKNOWN) {
		dwValue = TIMESRC_UNKNOWN;
	}

	*pdwSource = dwValue;

	return TRUE;
}

/**
 * Get IRIGB signal status
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - IRIGB signal source, the value is one of _PORT_LIST_.
 * @param  [out] dwStatus - A point to get IRIGB signal status, the value is one of _IRIG_SIGNAL_STATUS_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetSignalStatus(HANDLE hDev, DWORD dwSource, PDWORD pdwStatus)
{
	DWORD dwStatus;
	DWORD dwBytesReturned;
	BOOL bRet;
#ifdef WIN32
	bRet = DeviceIoControl(hDev, IOCTL_GET_TIMESRC_STATUS, NULL,
			0, &dwStatus, sizeof(dwStatus), &dwBytesReturned, NULL);
#else
	/* In Linux system, the return ( value == 0 ) means TRUE */
	bRet = (ioctl(hDev, IOCTL_GET_TIMESRC_STATUS, &dwStatus)==0) ? TRUE : FALSE;
#endif
	if (!bRet) {
		return bRet;
	}
	
	if ( dwSource == TIMESRC_FIBER) {
		if (dwStatus & INTSTS_BIT_IRIG0DE_OFF) {
			*pdwStatus = IRIG_STATUS_OFF_LINE;
		} else if (dwStatus & INTSTS_BIT_IRIG0DE_FRMERR) {
			*pdwStatus = IRIG_STATUS_FRAME_ERROR;
		} else if (dwStatus & INTSTS_BIT_IRIG0DE_PARERR) {
			*pdwStatus = IRIG_STATUS_PARITY_ERROR;
		} else if (dwStatus & INTSTS_BIT_IRIG0DE_DONE) {
			*pdwStatus = IRIG_STATUS_NORMAL;
		} else {
			*pdwStatus = IRIG_STATUS_UNKNOWN;
		}
	} else if ( dwSource == TIMESRC_PORT1) {
		if (dwStatus & INTSTS_BIT_IRIG1DE_OFF) {
			*pdwStatus = IRIG_STATUS_OFF_LINE;
		} else if (dwStatus & INTSTS_BIT_IRIG1DE_FRMERR) {
			*pdwStatus = IRIG_STATUS_FRAME_ERROR;
		} else if (dwStatus & INTSTS_BIT_IRIG1DE_PARERR) {
			*pdwStatus = IRIG_STATUS_PARITY_ERROR;
		} else if (dwStatus & INTSTS_BIT_IRIG1DE_DONE) {
			*pdwStatus = IRIG_STATUS_NORMAL;
		} else {
			*pdwStatus = IRIG_STATUS_UNKNOWN;
		}
	} else {
		SetLastError(ERROR_ACCESS_DENIED);
		bRet = FALSE;
	}

	return bRet;
}

/**
 * Set IRIGB input Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - the value is one of _RTC_SYNC_SOURCE_, should not be TIMESRC_FREERUN.
 * @param  [in] dwMode - the value is one of _PARITY_CHECK_MODE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetInputParityCheckMode(HANDLE hDev, DWORD dwSource, DWORD dwMode)
{
	BOOL bRet = TRUE;
	DWORD dwSetReg = 0;
	DWORD dwClrReg = 0;

	if ( dwSource == TIMESRC_FIBER ) {
		if (dwMode == PARITY_CHECK_EVEN) {
			dwClrReg = TMCON_BIT_IRIGDE0PARCHK_ODD |
						TMCON_BIT_IRIGDE0PARCHK_DIS;
		} else if (dwMode == PARITY_CHECK_ODD) {
			dwSetReg = TMCON_BIT_IRIGDE0PARCHK_ODD;
			dwClrReg = TMCON_BIT_IRIGDE0PARCHK_DIS;
		} else if (dwMode == PARITY_CHECK_NONE) {
			dwSetReg = TMCON_BIT_IRIGDE0PARCHK_DIS;
		} else {
			bRet = FALSE;
		}
	} else if ( dwSource == TIMESRC_PORT1 ) {
		if (dwMode == PARITY_CHECK_EVEN) {
			dwClrReg = TMCON_BIT_IRIGDE1PARCHK_ODD |
						TMCON_BIT_IRIGDE1PARCHK_DIS;
		} else if (dwMode == PARITY_CHECK_ODD) {
			dwSetReg = TMCON_BIT_IRIGDE1PARCHK_ODD;
			dwClrReg = TMCON_BIT_IRIGDE1PARCHK_DIS;
		} else if (dwMode == PARITY_CHECK_NONE) {
			dwSetReg = TMCON_BIT_IRIGDE1PARCHK_DIS;
		} else {
			bRet = FALSE;
		}
	} else {
		bRet = FALSE;
	}

	if (bRet) {
		bRet = mxirigb_setclrreg(hDev, TMCON, dwSetReg, dwClrReg );
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Get IRIGB input Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - the value is one of _RTC_SYNC_SOURCE_, should not be TIMESRC_FREERUN.
 * @param  [out] pdwMode - A point to get output parity check mode, the value is one of _PARITY_CHECK_MODE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetInputParityCheckMode(HANDLE hDev, DWORD dwSource, PDWORD pdwMode)
{
	BOOL bRet = TRUE;
	DWORD dwReg = 0;
	DWORD dwMode;

	bRet = mxirigb_getreg(hDev, TMCON, &dwReg );
	if ( dwSource == TIMESRC_FIBER ) {
		if (dwReg & TMCON_BIT_IRIGDE0PARCHK_DIS) {
			dwMode = PARITY_CHECK_NONE;
		} else if (dwReg & TMCON_BIT_IRIGDE0PARCHK_ODD) {
			dwMode = PARITY_CHECK_ODD;
		} else {
			dwMode = PARITY_CHECK_EVEN;
		}
	} else if ( dwSource == TIMESRC_PORT1 ) {
		if (dwReg & TMCON_BIT_IRIGDE1PARCHK_DIS) {
			dwMode = PARITY_CHECK_NONE;
		} else if (dwReg & TMCON_BIT_IRIGDE1PARCHK_ODD) {
			dwMode = PARITY_CHECK_ODD;
		} else {
			dwMode = PARITY_CHECK_EVEN;
		}
	} else {
		bRet = FALSE;
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	} else {
		*pdwMode = dwMode;
	}

	return bRet;
}

/**
 * Set IRIGB output Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwMode - the value is one of _PARITY_CHECK_MODE_, should not be PARITY_CHECK_NONE.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetOutputParityCheckMode(HANDLE hDev, DWORD dwMode)
{
	BOOL bRet = TRUE;
	DWORD dwSetReg = 0;
	DWORD dwClrReg = 0;

	if (dwMode == PARITY_CHECK_EVEN) {
		dwClrReg = TMCON_BIT_IRIGENPARCHK_ODD;
	} else if (dwMode == PARITY_CHECK_ODD) {
		dwSetReg = TMCON_BIT_IRIGENPARCHK_ODD;
	} else {
		bRet = FALSE;
	}

	if (bRet) {
		bRet = mxirigb_setclrreg(hDev, TMCON, dwSetReg, dwClrReg );
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Get IRIGB output Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwMode - A point to get output parity check mode.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetOutputParityCheckMode(HANDLE hDev, PDWORD pdwMode)
{
	BOOL bRet = TRUE;
	DWORD dwReg = 0;
	DWORD dwMode;

	bRet = mxirigb_getreg(hDev, TMCON, &dwReg );
	if (dwReg & TMCON_BIT_IRIGENPARCHK_ODD) {
		dwMode = PARITY_CHECK_ODD;
	} else {
		dwMode = PARITY_CHECK_EVEN;
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	} else {
		*pdwMode = dwMode;
	}

	return bRet;
}

/**
 * Set Pulse Per Second output width
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwMilliSecond - The pulse width per millisecond.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetPpsWidth(HANDLE hDev, DWORD dwMilliSecond)
{
	BOOL bRet = FALSE;

	if (dwMilliSecond<1000 && dwMilliSecond>=0) {
		bRet = mxirigb_setclrreg(hDev, PPSCON,
			dwMilliSecond << PPSCON_EN_PULSEWIDTH_BIT_S, 
			PPSCON_EN_PULSEWIDTH_MASK << PPSCON_EN_PULSEWIDTH_BIT_S );
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Get Pulse Per Second output width
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwMilliSecond - A point to get the pulse width per millisecond value.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetPpsWidth(HANDLE hDev, PDWORD pdwMilliSecond)
{
	BOOL bRet = FALSE;
	DWORD dwValue;

	bRet = mxirigb_getreg(hDev, PPSCON, &dwValue);
	if (bRet) {
		*pdwMilliSecond = (dwValue >> PPSCON_EN_PULSEWIDTH_BIT_S) &
						PPSCON_EN_PULSEWIDTH_MASK;
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Set input signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [in] dwType - Signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [in] invert - Signal inverse mode, if nonzero, invert the input signal.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetInputSignalType(HANDLE hDev, DWORD dwPort, DWORD dwType, BOOL invert)
{
	DWORD dwHwId;
	DWORD dwInvert = 0;
	BOOL bRet = FALSE;

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	if ((dwPort == PORT_FIBER) && (dwType == TYPE_TTL)) {
		if (dwHwId == DA_IRIGB_S) {
			/* Configure RX LED */
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_INP0 << NLED_P5_BIT_S),
				(NLED_MODE_MASK << NLED_P5_BIT_S) );

			if (!invert) {
				dwInvert = INPORTCON_BIT_INV0;
			}
			bRet = mxirigb_setclrreg(hDev, INPORTCON, 
				(INPSEL_INP0 << INPORTCON_IRIGDE0_BIT_S) |
					INPORTCON_BIT_IRIGDE0_DIS | dwInvert,
				(INPORTCON_MASK << INPORTCON_IRIGDE0_BIT_S) |
					INPORTCON_BIT_IRIGDE0_DIS | INPORTCON_BIT_INV0 );
		} else if (dwHwId == DE2_IRIGB_4DIO) {
			if (!invert) {
				dwInvert = INPORTCON_BIT_INV2;
			}
			bRet = mxirigb_setclrreg(hDev, INPORTCON, 
				(INPSEL_INP2 << INPORTCON_IRIGDE0_BIT_S) |
					INPORTCON_BIT_IRIGDE0_DIS | dwInvert,
				(INPORTCON_MASK << INPORTCON_IRIGDE0_BIT_S) |
					INPORTCON_BIT_IRIGDE0_DIS | INPORTCON_BIT_INV2 );
		}
	} else if ((dwPort == PORT_1) && (dwType == TYPE_TTL)) {
		if (dwHwId == DA_IRIGB_S) {
			/* Configure RX LED */
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_INP2 << NLED_P5_BIT_S),
				(NLED_MODE_MASK << NLED_P5_BIT_S) );
		} else if (dwHwId == DA_IRIGB_4DIO_PCI104) {
			bRet = mxirigb_setclrreg(hDev, PORTDAT, 0x0, 
				0x1 << (0 + PORTDATA_OUTPUT_BIT_S) );
		}

		if ((dwHwId == DA_IRIGB_S) || (dwHwId == DA_IRIGB_4DIO_PCI104)) {
			if (dwHwId == DA_IRIGB_4DIO_PCI104) {
				invert = !invert;
			}
			if (invert) {
				dwInvert = INPORTCON_BIT_INV2;
			}
			bRet = mxirigb_setclrreg(hDev, INPORTCON, 
				(INPSEL_INP2 << INPORTCON_IRIGDE1_BIT_S) |
					INPORTCON_BIT_IRIGDE1_DIS | dwInvert,
				(INPORTCON_MASK << INPORTCON_IRIGDE1_BIT_S) |
					INPORTCON_BIT_IRIGDE1_DIS | INPORTCON_BIT_INV2 );
		}
	} else if ((dwPort == PORT_1) && (dwType == TYPE_DIFFERENTIAL)) {
		if (dwHwId == DA_IRIGB_S) {
			/* Configure RX LED */
			mxirigb_setclrreg(hDev, NLEDCON, 
				(NLED_MODE_INP1 << NLED_P5_BIT_S),
				(NLED_MODE_MASK << NLED_P5_BIT_S) );
		} else if (dwHwId == DA_IRIGB_4DIO_PCI104) {
			bRet = mxirigb_setclrreg(hDev, 
				PORTDAT, 0x1 << (0 + PORTDATA_OUTPUT_BIT_S), 0x0 );
		}

		if ((dwHwId == DA_IRIGB_S) ||
			(dwHwId == DA_IRIGB_4DIO_PCI104) ||
			(dwHwId == DE2_IRIGB_4DIO) ) {
			if (invert) {
				dwInvert = INPORTCON_BIT_INV1;
			}
			bRet = mxirigb_setclrreg(hDev, INPORTCON, 
				(INPSEL_INP1 << INPORTCON_IRIGDE1_BIT_S) |
					INPORTCON_BIT_IRIGDE1_DIS | dwInvert,
				(INPORTCON_MASK << INPORTCON_IRIGDE1_BIT_S) |
					INPORTCON_BIT_IRIGDE1_DIS | INPORTCON_BIT_INV1 );
		}
	}

	// Need to check output mode: Is it "From Port Fiber or Port 1"?
	// Should reset output mode again.
	if (dwPort == PORT_FIBER) {
		if (dwHwId == DA_IRIGB_S) { // nothing to do
		} else if (dwHwId == DA_IRIGB_4DIO_PCI104) { // nothing to do
		}
	} else if (dwPort == PORT_1) {
		int portidx;
		DWORD dwOutType;
		DWORD dwOutMode;
		BOOL bOutInvert;

		if (dwHwId == DA_IRIGB_S) {
			for (portidx = PORT_1; portidx <= PORT_4; portidx++) {
				if ( mxIrigbGetOutputSignalType(hDev, portidx, &dwOutType, &dwOutMode, &bOutInvert) ) {
					if ( dwOutMode == MODE_FROM_PORT1_IN ) {
						mxIrigbSetOutputSignalType(hDev, portidx, dwOutType, dwOutMode, bOutInvert);
					}
				}
			}
		} else if (dwHwId == DA_IRIGB_4DIO_PCI104) {
			if ( mxIrigbGetOutputSignalType(hDev, PORT_1, &dwOutType, &dwOutMode, &bOutInvert) ) {
				if ( dwOutMode == MODE_FROM_PORT1_IN ) {
					mxIrigbSetOutputSignalType(hDev, PORT_1, dwOutType, dwOutMode, bOutInvert);
				}
			}
		}
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Get input signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [out] pdwType - A point to get IRIGB signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [out] pbInvert - A point to get signal mode, if nonzero, the signal is inverse.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetInputSignalType(HANDLE hDev, DWORD dwPort, PDWORD pdwType, PBOOL pbInvert)
{
	DWORD dwHwId;
	DWORD dwValue;
	BOOL bRet = FALSE;

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	if (dwPort == PORT_FIBER) {
		if (dwHwId == DA_IRIGB_S ) {
			bRet = mxirigb_getreg( hDev, INPORTCON, &dwValue);
			if ( bRet ) {
				if ( (dwValue & ((INPORTCON_MASK << INPORTCON_IRIGDE0_BIT_S) | INPORTCON_BIT_IRIGDE0_DIS)) ==
					((INPSEL_INP0 << INPORTCON_IRIGDE0_BIT_S) | INPORTCON_BIT_IRIGDE0_DIS)) {
					*pdwType = TYPE_TTL;

					if ( dwValue & INPORTCON_BIT_INV0 ) {
						*pbInvert = FALSE;
					} else {
						*pbInvert = TRUE;
					}
				} else {
					*pdwType = TYPE_UNKNOWN;
				}
			}
		} else if (dwHwId == DE2_IRIGB_4DIO) {
			bRet = mxirigb_getreg( hDev, INPORTCON, &dwValue);
			if ( bRet ) {
				if ( (dwValue & ((INPORTCON_MASK << INPORTCON_IRIGDE0_BIT_S) | INPORTCON_BIT_IRIGDE0_DIS)) ==
					((INPSEL_INP2 << INPORTCON_IRIGDE0_BIT_S) | INPORTCON_BIT_IRIGDE0_DIS)) {
					*pdwType = TYPE_TTL;

					if ( dwValue & INPORTCON_BIT_INV2 ) {
						*pbInvert = FALSE;
					} else {
						*pbInvert = TRUE;
					}
				} else {
					*pdwType = TYPE_UNKNOWN;
				}
			}
		}
	} else if (dwPort == PORT_1) {
		if ((dwHwId == DA_IRIGB_S) || (dwHwId == DA_IRIGB_4DIO_PCI104)) {
			bRet = mxirigb_getreg( hDev, INPORTCON, &dwValue);
			if ( bRet ) {
				if ( (dwValue & ((INPORTCON_MASK << INPORTCON_IRIGDE1_BIT_S) | INPORTCON_BIT_IRIGDE1_DIS)) ==
					((INPSEL_INP1 << INPORTCON_IRIGDE1_BIT_S) | INPORTCON_BIT_IRIGDE1_DIS)) {
					*pdwType = TYPE_DIFFERENTIAL;

					if ( dwValue & INPORTCON_BIT_INV1 ) {
						*pbInvert = TRUE;
					} else {
						*pbInvert = FALSE;
					}
				} else if ( (dwValue & ((INPORTCON_MASK << INPORTCON_IRIGDE1_BIT_S) | INPORTCON_BIT_IRIGDE1_DIS)) ==
					((INPSEL_INP2 << INPORTCON_IRIGDE1_BIT_S) | INPORTCON_BIT_IRIGDE1_DIS)) {
					*pdwType = TYPE_TTL;

					if ( dwValue & INPORTCON_BIT_INV2 ) {
						*pbInvert = TRUE;
					} else {
						*pbInvert = FALSE;
					}
					if (dwHwId == DA_IRIGB_4DIO_PCI104) {
						*pbInvert = !(*pbInvert);
					}
				} else {
					*pdwType = TYPE_UNKNOWN;
				}
			}
		} else if (dwHwId == DE2_IRIGB_4DIO) {
			bRet = mxirigb_getreg( hDev, INPORTCON, &dwValue);
			if ( bRet ) {
				if ( (dwValue & ((INPORTCON_MASK << INPORTCON_IRIGDE1_BIT_S) | INPORTCON_BIT_IRIGDE1_DIS)) ==
					((INPSEL_INP1 << INPORTCON_IRIGDE1_BIT_S) | INPORTCON_BIT_IRIGDE1_DIS)) {
					*pdwType = TYPE_DIFFERENTIAL;

					if ( dwValue & INPORTCON_BIT_INV1 ) {
						*pbInvert = TRUE;
					} else {
						*pbInvert = FALSE;
					}
				} else {
					*pdwType = TYPE_UNKNOWN;
				}
			}
		}
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Set output signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [in] dwType - Signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [in] dwMode - Signal output mode, the value is one of _OUTPUT_MODE_.
 * @param  [in] invert - Signal inverse mode, if nonzero, invert the output signal.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetOutputSignalType(HANDLE hDev, DWORD dwPort, DWORD dwType, DWORD dwMode, BOOL invert)
{
	DWORD dwHwId;
	DWORD dwOutportMode = -1;
	DWORD dwOutportModeShift = -1;
	DWORD dwInvert = 0;
	DWORD dwInvertKey = -1;
	DWORD dwPortdatShift = -1;

	BOOL bRet = FALSE;
	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	if (dwMode == MODE_FROM_FIBER_IN) {
		if (dwHwId == DA_IRIGB_S) {
			dwOutportMode = OUTPSEL_INP0;
		} else if (dwHwId == DE2_IRIGB_4DIO) {
			dwOutportMode = OUTPSEL_INP2;
		}
	} else if (dwMode == MODE_FROM_PORT1_IN) {
		if (dwHwId == DE2_IRIGB_4DIO) {
			dwOutportMode = OUTPSEL_INP1;
		} else {
			DWORD dwP1InputType;
			BOOL bP1InputInvert;
			if (mxIrigbGetInputSignalType( hDev, PORT_1, &dwP1InputType, &bP1InputInvert)) {
				if (dwP1InputType == TYPE_TTL) {
					dwOutportMode = OUTPSEL_INP2;
				} else {
					dwOutportMode = OUTPSEL_INP1;
				}
			} else {
				dwOutportMode = OUTPSEL_INP1;
			}
		}
	} else if (dwMode == MODE_IRIGB) {
		dwOutportMode = OUTPSEL_IRIGBEN;
	} else if (dwMode == MODE_PPS) {
		dwOutportMode = OUTPSEL_PPSEN;
	}

	if ( dwPort==PORT_1 ) {
		if (dwHwId==DA_IRIGB_S ||
			dwHwId==DA_IRIGB_4DIO_PCI104 ||
			dwHwId==DE2_IRIGB_4DIO ) {
			dwOutportModeShift = OUTPORTCON_P1_BIT_S;
			dwInvertKey = OUTPORTCON_BIT_INV1;
			dwPortdatShift = 12;
		}
	} else if (dwPort==PORT_2) {
		if (dwHwId==DA_IRIGB_S) {
			dwOutportModeShift = OUTPORTCON_P3_BIT_S;
			dwInvertKey = OUTPORTCON_BIT_INV3;
			dwPortdatShift = 14;
		}
	} else if (dwPort==PORT_3) {
		if (dwHwId==DA_IRIGB_S) {
			dwOutportModeShift = OUTPORTCON_P2_BIT_S;
			dwInvertKey = OUTPORTCON_BIT_INV2;
			dwPortdatShift = 13;
		}
	} else if (dwPort==PORT_4) {
		if (dwHwId==DA_IRIGB_S) {
			dwOutportModeShift = OUTPORTCON_P4_BIT_S;
			dwInvertKey = OUTPORTCON_BIT_INV4;
			dwPortdatShift = 15;
		}
	}

	if (dwOutportMode!=-1 && dwPortdatShift!=-1 && dwOutportModeShift!=-1) {
		if (invert) {
			dwInvert = dwInvertKey;
		}
		bRet = mxirigb_setclrreg(hDev, OUTPORTCON, 
			(dwOutportMode << dwOutportModeShift) | dwInvert,
			(OUTPORTCON_MASK << dwOutportModeShift) | dwInvertKey );
		if (!bRet) {
			return FALSE;
		}

		if ( dwType==TYPE_TTL ) {
			bRet = mxirigb_setclrreg(hDev, PORTDAT, 0x0, 
				0x1 << (dwPortdatShift + PORTDATA_OUTPUT_BIT_S) );
		} else if ( dwType==TYPE_DIFFERENTIAL ) {
			bRet = mxirigb_setclrreg(hDev, PORTDAT, 
				0x1 << (dwPortdatShift + PORTDATA_OUTPUT_BIT_S), 0x0 );
		} else {
			bRet = FALSE;
		}
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Get output signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [out] pdwType - A point to get signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [out] pdwMode - A point to get signal output mode, the value is one of _OUTPUT_MODE_.
 * @param  [out] pbInvert - A point to get signal mode, if nonzero, the signal is inverse.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetOutputSignalType(HANDLE hDev, DWORD dwPort, PDWORD pdwType, PDWORD pdwMode, PBOOL pbInvert)
{
	DWORD dwHwId;
	DWORD dwOutportcon;
	DWORD dwPortdat;
	DWORD dwOutportconShift = -1;
	DWORD dwInvertKey = -1;
	DWORD dwOutputModeShift = -1;
	BOOL bRet = FALSE;

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	if (!mxirigb_getreg(hDev, OUTPORTCON, &dwOutportcon )) {
		return FALSE;
	}

    if (!mxirigb_getreg(hDev, PORTDAT, &dwPortdat )) {
		return FALSE;
	}

	if (dwPort==PORT_1) {
		if (dwHwId==DA_IRIGB_S || dwHwId==DA_IRIGB_4DIO_PCI104 ||
			dwHwId==DE2_IRIGB_4DIO) {
			dwInvertKey = OUTPORTCON_BIT_INV1;
			dwOutportconShift = OUTPORTCON_P1_BIT_S;
			dwOutputModeShift = 12;
		}
	} else if (dwPort==PORT_2) {
		if (dwHwId==DA_IRIGB_S) {
			dwInvertKey = OUTPORTCON_BIT_INV3;
			dwOutportconShift = OUTPORTCON_P3_BIT_S;
			dwOutputModeShift = 14;
		}
	} else if (dwPort==PORT_3) {
		if (dwHwId==DA_IRIGB_S) {
			dwInvertKey = OUTPORTCON_BIT_INV2;
			dwOutportconShift = OUTPORTCON_P2_BIT_S;
			dwOutputModeShift = 13;
		}
	} else if (dwPort==PORT_4) {
		if (dwHwId==DA_IRIGB_S) {
			dwInvertKey = OUTPORTCON_BIT_INV4;
			dwOutportconShift = OUTPORTCON_P4_BIT_S;
			dwOutputModeShift = 15;
		}
	}

	if ( dwInvertKey!=-1 && dwOutportconShift!=-1 && dwOutputModeShift!=-1 ) {
		if (dwOutportcon&dwInvertKey) {
			*pbInvert = TRUE;
		} else {
			*pbInvert = FALSE;
		}

		if (OUTPSEL_IRIGBEN==((dwOutportcon >> dwOutportconShift)&OUTPORTCON_MASK)) {
			*pdwMode = MODE_IRIGB;
		} else if (OUTPSEL_PPSEN==((dwOutportcon >> dwOutportconShift)&OUTPORTCON_MASK)) {
			*pdwMode = MODE_PPS;
		} else if (OUTPSEL_INP0==((dwOutportcon >> dwOutportconShift)&OUTPORTCON_MASK)) {
			*pdwMode = MODE_FROM_FIBER_IN;
		} else if (OUTPSEL_INP1==((dwOutportcon >> OUTPORTCON_P1_BIT_S)&OUTPORTCON_MASK)) {
			*pdwMode = MODE_FROM_PORT1_IN; // TYPE_TTL
		} else if (OUTPSEL_INP2==((dwOutportcon >> OUTPORTCON_P1_BIT_S)&OUTPORTCON_MASK)) {
			*pdwMode = MODE_FROM_PORT1_IN; // TYPE_DIFFERENTIAL
		}

		if (0==(dwPortdat&(1 << (dwOutputModeShift+PORTDATA_OUTPUT_BIT_S)))) {
			*pdwType = TYPE_TTL;
		} else {
			*pdwType = TYPE_DIFFERENTIAL;
		}

		bRet = TRUE;
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return bRet;
}

/**
 * Set digital output signal
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - the port number (started from 0)
 * @param  [in] value - the port data, 1:HIGH, 0:LOW
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetDigitalOutputSignal(HANDLE hDev, DWORD dwPort, DWORD value)
{
	DWORD dwHwId;
	DWORD dwPortdatShift = -1;
	BOOL bRet = FALSE;

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	switch(dwPort) {
	case 0:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP8 ==> DO_0
			dwPortdatShift = 8;
		}
		break;

	case 1:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP9 ==> DO_1
			dwPortdatShift = 9;
		}
		break;

	case 2:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP10 ==> DO_2
			dwPortdatShift = 10;
		}
		break;

	case 3:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP11 ==> DO_3
			dwPortdatShift = 11;
		}
		break;
	}

	if (dwPortdatShift!=-1) {
		if (value==0) {
			bRet = mxirigb_setclrreg(hDev, PORTDAT, 0x0, 
				0x1<<(dwPortdatShift+ PORTDATA_OUTPUT_BIT_S) );
		} else {
			bRet = mxirigb_setclrreg(hDev, PORTDAT,
				0x1<<(dwPortdatShift+ PORTDATA_OUTPUT_BIT_S),
				0x0 );
		}
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

//#ifdef WIN32
	return bRet;
//#else
//  return ( bRet == 0 )? TRUE : FALSE ;
//#endif
}

/**
 * Get digital output signal
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - the port number (started from 0)
 * @param  [out] pValue - A point to get port data, 1:HIGH, 0:LOW
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetDigitalOutputSignal(HANDLE hDev, DWORD dwPort, PDWORD pValue)
{
	DWORD dwHwId;
	DWORD dwPortdatShift = -1;
	BOOL bRet = FALSE;

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	switch(dwPort) {
	case 0:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP8 ==> DO_0
			dwPortdatShift = 8;
		}
		break;

	case 1:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP9 ==> DO_1
			dwPortdatShift = 9;
		}
		break;

	case 2:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP10 ==> DO_2
			dwPortdatShift = 10;
		}
		break;

	case 3:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA OUTP11 ==> DO_3
			dwPortdatShift = 11;
		}
		break;
	}

	if (dwPortdatShift != -1) {
		DWORD dwPortdat;
		bRet = mxirigb_getreg(hDev, PORTDAT, &dwPortdat );
		if (bRet) {
			if ( 0 != (dwPortdat&(0x1<<(dwPortdatShift+PORTDATA_OUTPUT_BIT_S))) ) {
				*pValue = 1;
			} else {
				*pValue = 0;
			}
		}
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

//#ifdef WIN32
	return bRet;
//#else
//  return ( bRet == 0 )? TRUE : FALSE ;
//  return bRet;
//#endif
}

/**
 * Get digital input signal
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - the port number (started from 0)
 * @param  [out] pValue - A point to get port data, 1:HIGH, 0:LOW
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetDigitalInputSignal(HANDLE hDev, DWORD dwPort, PDWORD pValue)
{
	DWORD dwHwId;
	DWORD dwPortdatShift = -1;
	BOOL bRet = FALSE;

	if (!mxIrigbGetHardwareID(hDev, &dwHwId)) {
		return FALSE;
	}

	switch(dwPort) {
	case 0:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA INP10 ==> DI_0
			dwPortdatShift = 10;
		}
		break;

	case 1:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA INP7 ==> DI_1
			dwPortdatShift = 7;
		}
		break;

	case 2:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA INP6 ==> DI_2
			dwPortdatShift = 6;
		}
		break;

	case 3:
		if ((dwHwId == DA_IRIGB_4DIO_PCI104) || (dwHwId == DE2_IRIGB_4DIO)) {
			// FPGA INP5 ==> DI_3
			dwPortdatShift = 5;
		}
		break;
	}

	if (dwPortdatShift!=-1) {
		DWORD dwPortdat;
		bRet = mxirigb_getreg(hDev, PORTDAT, &dwPortdat );
		if (bRet) {
			if ( 0 != (dwPortdat&(0x1<<(dwPortdatShift+PORTDATA_INPUT_BIT_S))) ) {
				*pValue = 1;
			} else {
				*pValue = 0;
			}
		}
	}

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	}

//#ifdef WIN32
	return bRet;
//#else
//  return ( bRet == 0 )? TRUE : FALSE ;
//#endif
}

/**
 * Get FPGA firmware build date
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pValue - A point to get FPGA firmware build date, the value is BCD style yyyyMMdd, eg.:0x20140101
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetFpgaBuildDate(HANDLE hDev, PDWORD pValue)
{
	BOOL bRet = TRUE;
	DWORD dwReg = 0;

	bRet = mxirigb_getreg(hDev, DATECODE, &dwReg );

	if (!bRet) {
		SetLastError(ERROR_ACCESS_DENIED);
	} else {
		*pValue = dwReg;
	}

	return bRet;
}

#ifdef __cplusplus
}
#endif
