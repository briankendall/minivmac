#include "OSGCOMUI.h"
#include "OSGCOMUD.h"

#include <emscripten.h>

/*
	Operating System GLUe for Emscripten
*/

#ifdef WantOSGLUESC

/* --- some simple utilities --- */

GLOBALOSGLUPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount)
{
	(void) memcpy((char *)destPtr, (char *)srcPtr, byteCount);
}

/* --- prefs (mimics Basilisk II/SheepShaver prefs to allow code reuse) --- */

typedef struct {
    char *romPath;
    int diskPathCount;
    char **diskPaths;
    int screenWidth;
    int screenHeight;
} Prefs;

LOCALFUNC blnr LoadPrefs(Prefs *prefs) {
	FILE *prefsFile = fopen("/prefs", "r");
	if (NULL == prefsFile) {
		return falseblnr;
    }

	blnr v = trueblnr;
    prefs->romPath = NULL;
    prefs->diskPathCount = 0;
    prefs->screenWidth = 0;
    prefs->screenHeight = 0;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, prefsFile)) != -1) {
        // Trim trailing newline
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        char *input = line;
        char *key = strsep(&input, " ");
        if (NULL == key) {
            v = falseblnr;
            break;
        }
        if (strcmp(key, "rom") == 0) {
            prefs->romPath = strdup(input);
        } else if (strcmp(key, "disk") == 0) {
            if (0 == prefs->diskPathCount) {
                prefs->diskPaths = malloc(sizeof(char *));
            } else {
                prefs->diskPaths = realloc(prefs->diskPaths, sizeof(char *) * (prefs->diskPathCount + 1));
            }
            prefs->diskPaths[prefs->diskPathCount] = strdup(input);
            prefs->diskPathCount++;
        } else if (strcmp(key, "screen") == 0) {
            if (sscanf(input, "win/%d/%d", &prefs->screenWidth, &prefs->screenHeight) != 2) {
                v = falseblnr;
                break;
            }
        }
    }

    fclose(prefsFile);

    if (NULL != line) {
        free(line);
    }
    if (NULL == prefs->romPath ||
        0 == prefs->diskPathCount ||
        0 == prefs->screenWidth ||
        0 == prefs->screenHeight) {
        v = falseblnr;
    }
    return v;
}

/* --- control mode, parameter buffers and internationalization --- */

#include "INTLCHAR.h"

#include "COMOSGLU.h"

#include "PBUFSTDC.h"

#include "CONTROLM.h"

/* --- text translation --- */

#if IncludePbufs
/* TODO: this is a table for Windows, switch to UTF-8 */
LOCALVAR const ui3b Native2MacRomanTab[] = {
	0xAD, 0xB0, 0xE2, 0xC4, 0xE3, 0xC9, 0xA0, 0xE0,
	0xF6, 0xE4, 0xB6, 0xDC, 0xCE, 0xB2, 0xB3, 0xB7,
	0xB8, 0xD4, 0xD5, 0xD2, 0xD3, 0xA5, 0xD0, 0xD1,
	0xF7, 0xAA, 0xC5, 0xDD, 0xCF, 0xB9, 0xC3, 0xD9,
	0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0xBA, 0xA4,
	0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0xBD, 0xA8, 0xF8,
	0xA1, 0xB1, 0xC6, 0xD7, 0xAB, 0xB5, 0xA6, 0xE1,
	0xFC, 0xDA, 0xBC, 0xC8, 0xDE, 0xDF, 0xF0, 0xC0,
	0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82,
	0xE9, 0x83, 0xE6, 0xE8, 0xED, 0xEA, 0xEB, 0xEC,
	0xF5, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0xF9,
	0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0xFA, 0xFB, 0xA7,
	0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D,
	0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
	0xFD, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6,
	0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0xFE, 0xFF, 0xD8
};
#endif

#if IncludePbufs
LOCALFUNC tMacErr NativeTextToMacRomanPbuf(char *x, tPbuf *r)
{
	if (NULL == x) {
		return mnvm_miscErr;
	} else {
		ui3p p;
		ui5b L = strlen(x);

		p = (ui3p)malloc(L);
		if (NULL == p) {
			return mnvm_miscErr;
		} else {
			ui3b *p0 = (ui3b *)x;
			ui3b *p1 = (ui3b *)p;
			int i;

			for (i = L; --i >= 0; ) {
				ui3b v = *p0++;
				if (v >= 128) {
					v = Native2MacRomanTab[v - 128];
				} else if (10 == v) {
					v = 13;
				}
				*p1++ = v;
			}

			return PbufNewFromPtr(p, L, r);
		}
	}
}
#endif

#if IncludePbufs
/* TODO: this is a table for Windows, switch to UTF-8 */
LOCALVAR const ui3b MacRoman2NativeTab[] = {
	0xC4, 0xC5, 0xC7, 0xC9, 0xD1, 0xD6, 0xDC, 0xE1,
	0xE0, 0xE2, 0xE4, 0xE3, 0xE5, 0xE7, 0xE9, 0xE8,
	0xEA, 0xEB, 0xED, 0xEC, 0xEE, 0xEF, 0xF1, 0xF3,
	0xF2, 0xF4, 0xF6, 0xF5, 0xFA, 0xF9, 0xFB, 0xFC,
	0x86, 0xB0, 0xA2, 0xA3, 0xA7, 0x95, 0xB6, 0xDF,
	0xAE, 0xA9, 0x99, 0xB4, 0xA8, 0x80, 0xC6, 0xD8,
	0x81, 0xB1, 0x8D, 0x8E, 0xA5, 0xB5, 0x8A, 0x8F,
	0x90, 0x9D, 0xA6, 0xAA, 0xBA, 0xAD, 0xE6, 0xF8,
	0xBF, 0xA1, 0xAC, 0x9E, 0x83, 0x9A, 0xB2, 0xAB,
	0xBB, 0x85, 0xA0, 0xC0, 0xC3, 0xD5, 0x8C, 0x9C,
	0x96, 0x97, 0x93, 0x94, 0x91, 0x92, 0xF7, 0xB3,
	0xFF, 0x9F, 0xB9, 0xA4, 0x8B, 0x9B, 0xBC, 0xBD,
	0x87, 0xB7, 0x82, 0x84, 0x89, 0xC2, 0xCA, 0xC1,
	0xCB, 0xC8, 0xCD, 0xCE, 0xCF, 0xCC, 0xD3, 0xD4,
	0xBE, 0xD2, 0xDA, 0xDB, 0xD9, 0xD0, 0x88, 0x98,
	0xAF, 0xD7, 0xDD, 0xDE, 0xB8, 0xF0, 0xFD, 0xFE
};
#endif

#if IncludePbufs
LOCALFUNC blnr MacRomanTextToNativePtr(tPbuf i, blnr IsFileName,
	ui3p *r)
{
	ui3p p;
	void *Buffer = PbufDat[i];
	ui5b L = PbufSize[i];

	p = (ui3p)malloc(L + 1);
	if (p != NULL) {
		ui3b *p0 = (ui3b *)Buffer;
		ui3b *p1 = (ui3b *)p;
		int j;

		if (IsFileName) {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x < 32) {
					x = '-';
				} else if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else {
					switch (x) {
						case '/':
						case '<':
						case '>':
						case '|':
						case ':':
							x = '-';
						default:
							break;
					}
				}
				*p1++ = x;
			}
			if ('.' == p[0]) {
				p[0] = '-';
			}
		} else {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else if (13 == x) {
					x = '\n';
				}
				*p1++ = x;
			}
		}
		*p1 = 0;

		*r = p;
		return trueblnr;
	}
	return falseblnr;
}
#endif

/* --- drives --- */

#define NotAfileRef NULL

LOCALVAR FILE *Drives[NumDrives]; /* open disk image files */
#if IncludeSonyGetName || IncludeSonyNew
LOCALVAR char *DriveNames[NumDrives];
#endif

LOCALPROC InitDrives(void)
{
	/*
		This isn't really needed, Drives[i] and DriveNames[i]
		need not have valid values when not vSonyIsInserted[i].
	*/
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		Drives[i] = NotAfileRef;
#if IncludeSonyGetName || IncludeSonyNew
		DriveNames[i] = NULL;
#endif
	}
}

GLOBALOSGLUFUNC tMacErr vSonyTransfer(blnr IsWrite, ui3p Buffer,
	tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count,
	ui5r *Sony_ActCount)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	ui5r NewSony_Count = 0;

	if (0 == fseek(refnum, Sony_Start, SEEK_SET)) {
		if (IsWrite) {
			NewSony_Count = fwrite(Buffer, 1, Sony_Count, refnum);
		} else {
			NewSony_Count = fread(Buffer, 1, Sony_Count, refnum);
		}

		if (NewSony_Count == Sony_Count) {
			err = mnvm_noErr;
		}
	}

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = NewSony_Count;
	}

	if (IsWrite) {
		EM_ASM_({ workerApi.didDiskWrite($0, $1); }, Sony_Start, Sony_Count);
	}

	return err; /*& figure out what really to return &*/
}

GLOBALOSGLUFUNC tMacErr vSonyGetSize(tDrive Drive_No, ui5r *Sony_Count)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	long v;

	if (0 == fseek(refnum, 0, SEEK_END)) {
		v = ftell(refnum);
		if (v >= 0) {
			*Sony_Count = v;
			err = mnvm_noErr;
		}
	}

	return err; /*& figure out what really to return &*/
}

LOCALFUNC tMacErr vSonyEject0(tDrive Drive_No, blnr deleteit)
{
	FILE *refnum = Drives[Drive_No];

	DiskEjectedNotify(Drive_No);

	fclose(refnum);
	Drives[Drive_No] = NotAfileRef; /* not really needed */

#if IncludeSonyGetName || IncludeSonyNew
	{
		char *s = DriveNames[Drive_No];
		if (NULL != s) {
			if (deleteit) {
				remove(s);
			}
			free(s);
			DriveNames[Drive_No] = NULL; /* not really needed */
		}
	}
#endif

	return mnvm_noErr;
}

GLOBALOSGLUFUNC tMacErr vSonyEject(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, falseblnr);
}

#if IncludeSonyNew
GLOBALOSGLUFUNC tMacErr vSonyEjectDelete(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, trueblnr);
}
#endif

LOCALPROC UnInitDrives(void)
{
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		if (vSonyIsInserted(i)) {
			(void) vSonyEject(i);
		}
	}
}

#if IncludeSonyGetName
GLOBALOSGLUFUNC tMacErr vSonyGetName(tDrive Drive_No, tPbuf *r)
{
	char *drivepath = DriveNames[Drive_No];
	if (NULL == drivepath) {
		return mnvm_miscErr;
	} else {
		char *s = strrchr(drivepath, '/');
		if (NULL == s) {
			s = drivepath;
		} else {
			++s;
		}
		return NativeTextToMacRomanPbuf(s, r);
	}
}
#endif

LOCALFUNC blnr Sony_Insert0(FILE *refnum, blnr locked,
	char *drivepath)
{
	tDrive Drive_No;
	blnr IsOk = falseblnr;

	if (! FirstFreeDisk(&Drive_No)) {
		MacMsg(kStrTooManyImagesTitle, kStrTooManyImagesMessage,
			falseblnr);
	} else {
        Drives[Drive_No] = refnum;
        DiskInsertNotify(Drive_No, locked);

#if IncludeSonyGetName || IncludeSonyNew
        {
            ui5b L = strlen(drivepath);
            char *p = malloc(L + 1);
            if (p != NULL) {
                (void) memcpy(p, drivepath, L + 1);
            }
            DriveNames[Drive_No] = p;
        }
#endif

        IsOk = trueblnr;
	}

	if (! IsOk) {
		fclose(refnum);
	}

	return IsOk;
}

LOCALFUNC blnr Sony_Insert1(char *drivepath, blnr silentfail)
{
	blnr locked = falseblnr;
	FILE *refnum = fopen(drivepath, "rb+");
	if (NULL == refnum) {
		locked = trueblnr;
		refnum = fopen(drivepath, "rb");
	}
	if (NULL == refnum) {
		if (! silentfail) {
			MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
		}
	} else {
		return Sony_Insert0(refnum, locked, drivepath);
	}
	return falseblnr;
}

LOCALFUNC blnr LoadInitialImages(Prefs prefs)
{
    for (int i = 0; i < prefs.diskPathCount && Sony_Insert1(prefs.diskPaths[i], falseblnr); ++i) {
        /* stop on first error (including file not found) */
    }

	return trueblnr;
}


/* --- ROM --- */

LOCALFUNC blnr LoadMacRom(Prefs prefs)
{
	blnr v;
	FILE *ROM_File;
	int File_Size;

	ROM_File = fopen(prefs.romPath, "rb");
	if (NULL == ROM_File) {
		v = falseblnr;
	} else {
		File_Size = fread(ROM, 1, kROM_Size, ROM_File);
		if (kROM_Size != File_Size) {
			if (feof(ROM_File)) {
				MacMsgOverride(kStrShortROMTitle,
					kStrShortROMMessage);
                v = falseblnr;
			} else {
				MacMsgOverride(kStrNoReadROMTitle,
					kStrNoReadROMMessage);
                v = falseblnr;
			}
		} else {
			v = mnvm_noErr == ROM_IsValid();
		}
		fclose(ROM_File);
	}

	return v;
}

/* --- video out --- */

#ifndef UseColorImage
#define UseColorImage (0 != WantColorImage)
#endif

LOCALVAR uint32_t *BrowserFramebuffer;
LOCALVAR uint32_t BWTo32bitLUT[256][8];

LOCALPROC HaveChangedScreenBuff(ui4r top, ui4r left,
	ui4r bottom, ui4r right)
{
	ui3b *macFramebuffer = (ui3b *)GetCurDrawBuff();

	// TODO: check UseColorMode and UseColorImage
	ui4r leftByte = left / 8;
	ui4r rightByte = (right + 7) / 8;

	for (int y = top; y < bottom; y++) {
		for (int xByte = leftByte; xByte < rightByte; xByte++) {
			ui3b *p = macFramebuffer + y * vMacScreenByteWidth + xByte;
			memcpy(BrowserFramebuffer + y * vMacScreenWidth + xByte * 8, BWTo32bitLUT[*p], 8 * sizeof(uint32_t));
		}
	}

	EM_ASM_(
		{ workerApi.blit($0, $1, {top: $2, left: $3, bottom: $4, right: $5}); },
		BrowserFramebuffer,
		vMacScreenWidth * vMacScreenHeight * 4,
		top, left, bottom, right);
}

LOCALPROC MyDrawChangesAndClear(void)
{
	if (ScreenChangedBottom > ScreenChangedTop) {
		HaveChangedScreenBuff(ScreenChangedTop, ScreenChangedLeft,
			ScreenChangedBottom, ScreenChangedRight);
		// ScreenClearChanges is normally invoked here, but it does the same
		// thing as ScreenChangedAll (in the v37 beta, though not in the v36
		// stable release), and thus marks the entire screen as  dirty. Do our
		// own empty initial state instead.
		ScreenChangedTop = vMacScreenHeight;
		ScreenChangedBottom = 0;
		ScreenChangedLeft = vMacScreenWidth;
		ScreenChangedRight = 0;
	} else {
		// Still need to signal to the JS that a blit would have been done, so
		// that we can compute the permitted idle time.
	    EM_ASM_({ workerApi.blit(0, 0); });
	}
}

GLOBALOSGLUPROC DoneWithDrawingForTick(void)
{
#if EnableFSMouseMotion
	if (HaveMouseMotion) {
		AutoScrollScreen();
	}
#endif
	MyDrawChangesAndClear();
}

/* --- time, date, location --- */

#define dbglog_TimeStuff (0 && dbglog_HAVE)

LOCALVAR ui5b TrueEmulatedTime = 0;

#include "DATE2SEC.h"

#define TicksPerSecond 1000000

LOCALVAR blnr HaveTimeDelta = falseblnr;
LOCALVAR ui5b TimeDelta;

LOCALVAR ui5b NewMacDateInSeconds;

LOCALVAR ui5b LastTimeSec;
LOCALVAR ui5b LastTimeUsec;

LOCALPROC GetCurrentTicks(void)
{
	struct timeval t;

	gettimeofday(&t, NULL);
	if (! HaveTimeDelta) {
		time_t Current_Time;
		struct tm *s;

		(void) time(&Current_Time);
		s = localtime(&Current_Time);
		TimeDelta = Date2MacSeconds(s->tm_sec, s->tm_min, s->tm_hour,
			s->tm_mday, 1 + s->tm_mon, 1900 + s->tm_year) - t.tv_sec;
#if 0 && AutoTimeZone /* how portable is this ? */
		CurMacDelta = ((ui5b)(s->tm_gmtoff) & 0x00FFFFFF)
			| ((s->tm_isdst ? 0x80 : 0) << 24);
#endif
		HaveTimeDelta = trueblnr;
	}

	NewMacDateInSeconds = t.tv_sec + TimeDelta;
	LastTimeSec = (ui5b)t.tv_sec;
	LastTimeUsec = (ui5b)t.tv_usec;
}

#define MyInvTimeStep 16626 /* TicksPerSecond / 60.14742 */

LOCALVAR ui5b NextTimeSec;
LOCALVAR ui5b NextTimeUsec;

LOCALPROC IncrNextTime(void)
{
	NextTimeUsec += MyInvTimeStep;
	if (NextTimeUsec >= TicksPerSecond) {
		NextTimeUsec -= TicksPerSecond;
		NextTimeSec += 1;
	}
}

LOCALPROC InitNextTime(void)
{
	NextTimeSec = LastTimeSec;
	NextTimeUsec = LastTimeUsec;
	IncrNextTime();
}

LOCALPROC StartUpTimeAdjust(void)
{
	GetCurrentTicks();
	InitNextTime();
}

LOCALFUNC si5b GetTimeDiff(void)
{
	return ((si5b)(LastTimeSec - NextTimeSec)) * TicksPerSecond
		+ ((si5b)(LastTimeUsec - NextTimeUsec));
}

LOCALPROC UpdateTrueEmulatedTime(void)
{
	si5b TimeDiff;

	GetCurrentTicks();

	TimeDiff = GetTimeDiff();
	if (TimeDiff >= 0) {
		if (TimeDiff > 16 * MyInvTimeStep) {
			/* emulation interrupted, forget it */
			++TrueEmulatedTime;
			InitNextTime();

#if dbglog_TimeStuff
			dbglog_writelnNum("emulation interrupted",
				TrueEmulatedTime);
#endif
		} else {
			do {
				++TrueEmulatedTime;
				IncrNextTime();
				TimeDiff -= TicksPerSecond;
			} while (TimeDiff >= 0);
		}
	} else if (TimeDiff < - 16 * MyInvTimeStep) {
		/* clock goofed if ever get here, reset */
#if dbglog_TimeStuff
		dbglog_writeln("clock set back");
#endif

		InitNextTime();
	}
}

LOCALFUNC blnr CheckDateTime(void)
{
	if (CurMacDateInSeconds != NewMacDateInSeconds) {
		CurMacDateInSeconds = NewMacDateInSeconds;
		return trueblnr;
	} else {
		return falseblnr;
	}
}

LOCALFUNC blnr InitLocationDat(void)
{
	GetCurrentTicks();
	CurMacDateInSeconds = NewMacDateInSeconds;

	return trueblnr;
}

/* --- disk images -- */

EM_JS(char*, consumeDiskImagePath, (void), {
	const diskImagePath = workerApi.consumeDiskImagePath();
	if (!diskImagePath || !diskImagePath.length) {
		return 0;
	}
	const diskImagePathLength = lengthBytesUTF8(diskImagePath) + 1;
	const diskImagePathCstr = _malloc(diskImagePathLength);
	stringToUTF8(diskImagePath, diskImagePathCstr, diskImagePathLength);
	return diskImagePathCstr;
});

LOCALVAR ui5b LastDiskImageCheckTime;

LOCALFUNC void HandleDiskImages(void)
{
	ui5b currentTime = LastTimeSec * TicksPerSecond + LastTimeUsec;
	if (currentTime - LastDiskImageCheckTime < 100000) {
		return;
	}
	LastDiskImageCheckTime = currentTime;
	char *diskImagePath = consumeDiskImagePath();
	if (diskImagePath) {
		Sony_Insert1(diskImagePath, trueblnr);
		free(diskImagePath);
	}
}

/* --- clipboard --- */

#if IncludeHostTextClipExchange
LOCALVAR ui3p MyClipBuffer = NULL;
#endif

#if IncludeHostTextClipExchange
LOCALPROC FreeMyClipBuffer(void)
{
	if (MyClipBuffer != NULL) {
		free(MyClipBuffer);
		MyClipBuffer = NULL;
	}
}
#endif

#if IncludeHostTextClipExchange
GLOBALOSGLUFUNC tMacErr HTCEexport(tPbuf i)
{
	tMacErr err = mnvm_miscErr;

	FreeMyClipBuffer();
	if (MacRomanTextToNativePtr(i, falseblnr,
		&MyClipBuffer))
	{
        // TODO: export clipboard
		err = mnvm_noErr;
	}

	PbufDispose(i);

	return err;
}
#endif

#if IncludeHostTextClipExchange
LOCALPROC HTCEimport_do(void)
{
	// TODO: clipboard import
}
#endif

#if IncludeHostTextClipExchange
GLOBALOSGLUFUNC tMacErr HTCEimport(tPbuf *r)
{
	HTCEimport_do();

	return NativeTextToMacRomanPbuf((char *)MyClipBuffer, r);
}
#endif

/* --- SavedTasks --- */

LOCALVAR blnr CurSpeedStopped = trueblnr;

LOCALPROC LeaveSpeedStopped(void)
{
#if MySoundEnabled
	MySound_Start();
#endif

	StartUpTimeAdjust();
}

LOCALPROC EnterSpeedStopped(void)
{
#if MySoundEnabled
	MySound_Stop();
#endif
}

LOCALPROC CheckForSavedTasks(void)
{
	if (MyEvtQNeedRecover) {
		MyEvtQNeedRecover = falseblnr;

		/* attempt cleanup, MyEvtQNeedRecover may get set again */
		MyEvtQTryRecoverFromFull();
	}

	if (RequestMacOff) {
		RequestMacOff = falseblnr;
		if (AnyDiskInserted()) {
			MacMsgOverride(kStrQuitWarningTitle,
				kStrQuitWarningMessage);
		} else {
			ForceMacOff = trueblnr;
		}
	}

	if (ForceMacOff) {
		return;
	}

	if (CurSpeedStopped != SpeedStopped)
	{
		CurSpeedStopped = ! CurSpeedStopped;
		if (CurSpeedStopped) {
			EnterSpeedStopped();
		} else {
			LeaveSpeedStopped();
		}
	}

	if ((nullpr != SavedBriefMsg) & ! MacMsgDisplayed) {
		MacMsgDisplayOn();
	}

#if NeedRequestIthDisk
	if (0 != RequestIthDisk) {
		Sony_InsertIth(RequestIthDisk);
		RequestIthDisk = 0;
	}
#endif

	if (NeedWholeScreenDraw) {
		NeedWholeScreenDraw = falseblnr;
		ScreenChangedAll();
	}
}

/* --- input --- */

LOCALPROC ReadJSInput()
{
    int lock = EM_ASM_INT_V({ return workerApi.acquireInputLock(); });
    if (!lock) {
        return;
    }
    int mouseButtonState = EM_ASM_INT_V({
        return workerApi.getInputValue(workerApi.InputBufferAddresses.mouseButtonStateAddr);
    });

    if (mouseButtonState > -1) {
        MyMouseButtonSet(mouseButtonState == 0 ? falseblnr : trueblnr);
    }

    int hasMousePosition = EM_ASM_INT_V({
        return workerApi.getInputValue(workerApi.InputBufferAddresses.mousePositionFlagAddr);
    });
    if (hasMousePosition) {
        int mouseX = EM_ASM_INT_V({
            return workerApi.getInputValue(workerApi.InputBufferAddresses.mousePositionXAddr);
        });
        int mouseY = EM_ASM_INT_V({
            return workerApi.getInputValue(workerApi.InputBufferAddresses.mousePositionYAddr);
        });

        MyMousePositionSet(mouseX, mouseY);
    }

    int hasKeyEvent = EM_ASM_INT_V({
        return workerApi.getInputValue(workerApi.InputBufferAddresses.keyEventFlagAddr);
    });
    if (hasKeyEvent) {
        int keycode = EM_ASM_INT_V({
            return workerApi.getInputValue(workerApi.InputBufferAddresses.keyCodeAddr);
        });

        int keystate = EM_ASM_INT_V({
            return workerApi.getInputValue(workerApi.InputBufferAddresses.keyStateAddr);
        });

        Keyboard_UpdateKeyMap2(keycode, keystate == 0 ? falseblnr : trueblnr);
    }

    // TODO: ethernet interrupt

    EM_ASM({ workerApi.releaseInputLock(); });
}

/* --- main program flow --- */

GLOBALOSGLUFUNC blnr ExtraTimeNotOver(void)
{
	UpdateTrueEmulatedTime();
	return TrueEmulatedTime == OnTrueTime;
}

GLOBALOSGLUPROC WaitForNextTick(void)
{
label_retry:
	CheckForSavedTasks();

	if (ForceMacOff) {
		return;
	}

	if (CurSpeedStopped) {
		DoneWithDrawingForTick();
		goto label_retry;
	}

	if (ExtraTimeNotOver()) {
        si5b TimeDiff = GetTimeDiff();
		if (TimeDiff < 0) {
			EM_ASM_(
				{ workerApi.sleep($0); },
				-((double) TimeDiff)/TicksPerSecond);
        }
		goto label_retry;
	}

	if (CheckDateTime()) {
#if MySoundEnabled
		MySound_SecondNotify();
#endif
	}

	HandleDiskImages();
    ReadJSInput();

	OnTrueTime = TrueEmulatedTime;
}

/* --- platform independent code can be thought of as going here --- */

#include "PROGMAIN.h"

LOCALPROC ZapOSGLUVars(void)
{
	InitDrives();
}

LOCALPROC ReserveAllocAll(void)
{
#if dbglog_HAVE
	dbglog_ReserveAlloc();
#endif
	ReserveAllocOneBlock(&ROM, kROM_Size, 5, falseblnr);

	ReserveAllocOneBlock(&screencomparebuff,
		vMacScreenNumBytes, 5, trueblnr);
#if UseControlKeys
	ReserveAllocOneBlock(&CntrlDisplayBuff,
		vMacScreenNumBytes, 5, falseblnr);
#endif

#if MySoundEnabled
	ReserveAllocOneBlock((ui3p *)&TheSoundBuffer,
		dbhBufferSize, 5, falseblnr);
#endif

	EmulationReserveAlloc();
}

LOCALFUNC blnr AllocMyMemory(void)
{
	uimr n;
	blnr IsOk = falseblnr;

	ReserveAllocOffset = 0;
	ReserveAllocBigBlock = nullpr;
	ReserveAllocAll();
	n = ReserveAllocOffset;
	ReserveAllocBigBlock = (ui3p)calloc(1, n);
	if (NULL == ReserveAllocBigBlock) {
		MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
	} else {
		ReserveAllocOffset = 0;
		ReserveAllocAll();
		if (n != ReserveAllocOffset) {
			/* oops, program error */
		} else {
			IsOk = trueblnr;
		}
	}

	return IsOk;
}

LOCALPROC UnallocMyMemory(void)
{
	if (nullpr != ReserveAllocBigBlock) {
		free((char *)ReserveAllocBigBlock);
	}
}

/* --- basic dialogs --- */

LOCALPROC CheckSavedMacMsg(void)
{
	if (nullpr != SavedBriefMsg) {
        printf("Error message (brief): %s\n", SavedBriefMsg);
    }
	if (nullpr != SavedLongMsg) {
        printf("Error message (long): %s\n", SavedLongMsg);
    }
}

/* --- main window creation and disposal --- */

LOCALFUNC blnr Screen_Init(void)
{

    BrowserFramebuffer = (uint32_t *)malloc(vMacScreenWidth * vMacScreenHeight * 4);
	for (size_t byte = 0; byte < 256; byte++) {
		for (size_t bit = 0; bit < 8; bit++) {
			BWTo32bitLUT[byte][7 - bit] = (byte & (1 << bit)) ? 0x00000000 : 0xFFFFFFFF;
		}
	}

    printf("Screen_Init(%dx%d, depth=%d)\n", vMacScreenWidth, vMacScreenHeight, vMacScreenDepth);
    EM_ASM_({ workerApi.didOpenVideo($0, $1); }, vMacScreenWidth, vMacScreenHeight);

	InitKeyCodes();
	ScreenClearChanges();

	return trueblnr;
}

LOCALFUNC void Screen_UnInit(void)
{
    free(BrowserFramebuffer);
}

LOCALFUNC blnr InitOSGLU(void)
{
	blnr IsOk = falseblnr;
    Prefs prefs;

	if (AllocMyMemory())
#if MySoundEnabled
	if (MySound_Init())
		/* takes a while to stabilize, do as soon as possible */
#endif
    if (LoadPrefs(&prefs))
	if (LoadMacRom(prefs))
	if (LoadInitialImages(prefs))
#if UseActvCode
	if (ActvCodeInit())
#endif
	if (InitLocationDat())
	if (Screen_Init())
#if EmLocalTalk
	if (EntropyGather())
	if (InitLocalTalk())
#endif
	if (WaitForRom())
	{
		IsOk = trueblnr;
	}

	return IsOk;
}

LOCALPROC UnInitOSGLU(void)
{
	if (MacMsgDisplayed) {
		MacMsgDisplayOff();
	}

    // No-op, but suppresses a warning about this being an unused function.
    DisconnectKeyCodes2();

#if EmLocalTalk
	UnInitLocalTalk();
#endif

#if MySoundEnabled
	MySound_Stop();
#endif
#if MySoundEnabled
	MySound_UnInit();
#endif
#if IncludeHostTextClipExchange
	FreeMyClipBuffer();
#endif
#if IncludePbufs
	UnInitPbufs();
#endif
	UnInitDrives();

	Screen_UnInit();

	CheckSavedMacMsg();

	UnallocMyMemory();
}

int main(void)
{
	ZapOSGLUVars();
	if (InitOSGLU()) {
		ProgramMain();
	}
    printf("Exiting\n");
	UnInitOSGLU();

	return 0;
}

#endif /* WantOSGLUESC */
