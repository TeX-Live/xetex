/*
**********************************************************************
*   Copyright (C) 2002-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  iotest.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002feb21
*   created by: George Rhoten
*/


#include "unicode/ustdio.h"
#include "unicode/ustream.h"
#include "unicode/uclean.h"

#include "unicode/ucnv.h"
#include "unicode/uchar.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "ustr_cnv.h"
#include "iotest.h"
#include "unicode/tstdtmod.h"

#if U_IOSTREAM_SOURCE >= 199711
#include <iostream>
#ifdef U_LINUX
#define USE_SSTREAM 1
#include <sstream>
#else
// <strstream> is deprecated on some platforms, and the compiler complains very loudly if you use it.
#include <strstream>
#endif
using namespace std;
#elif U_IOSTREAM_SOURCE >= 198506
#include <iostream.h>
#include <strstream.h>
#endif

#include <string.h>
#include <stdlib.h>

U_CDECL_BEGIN
#ifdef WIN32
const UChar NEW_LINE[] = {0x0d,0x0a,0};
const char C_NEW_LINE[] = {0x0d,0x0a,0};
#define UTF8_NEW_LINE "\x0d\x0a"
#else
const UChar NEW_LINE[] = {0x0a,0};
const char C_NEW_LINE[] = {'\n',0};
#define UTF8_NEW_LINE "\x0a"
#endif
U_CDECL_END

class DataDrivenLogger : public TestLog {
    static const char* fgDataDir;
    static char *fgTestDataPath;

public:
    static void cleanUp() {
        if (fgTestDataPath) {
            free(fgTestDataPath);
            fgTestDataPath = NULL;
        }
    }
    virtual void errln( const UnicodeString &message ) {
        char buffer[4000];
        message.extract(0, message.length(), buffer, sizeof(buffer));
        buffer[3999] = 0; /* NULL terminate */
        log_err(buffer);
    }

    static const char * pathToDataDirectory(void)
    {

        if(fgDataDir != NULL) {
            return fgDataDir;
        }

        /* U_TOPSRCDIR is set by the makefiles on UNIXes when building cintltst and intltst
        //              to point to the top of the build hierarchy, which may or
        //              may not be the same as the source directory, depending on
        //              the configure options used.  At any rate,
        //              set the data path to the built data from this directory.
        //              The value is complete with quotes, so it can be used
        //              as-is as a string constant.
        */
    #if defined (U_TOPSRCDIR)
        {
            fgDataDir = U_TOPSRCDIR  U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
        }
    #else

        /* On Windows, the file name obtained from __FILE__ includes a full path.
        *             This file is "wherever\icu\source\test\cintltst\cintltst.c"
        *             Change to    "wherever\icu\source\data"
        */
        {
            static char p[sizeof(__FILE__) + 10];
            char *pBackSlash;
            int i;

            strcpy(p, __FILE__);
            /* We want to back over three '\' chars.                            */
            /*   Only Windows should end up here, so looking for '\' is safe.   */
            for (i=1; i<=3; i++) {
                pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
                if (pBackSlash != NULL) {
                    *pBackSlash = 0;        /* Truncate the string at the '\'   */
                }
            }

            if (pBackSlash != NULL) {
                /* We found and truncated three names from the path.
                *  Now append "source\data" and set the environment
                */
                strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING );
                fgDataDir = p;
            }
            else {
                /* __FILE__ on MSVC7 does not contain the directory */
                FILE *file = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
                if (file) {
                    fclose(file);
                    fgDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
                }
                else {
                    fgDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
                }
            }
        }
    #endif

        return fgDataDir;

    }

    static const char* loadTestData(UErrorCode& err){
        if( fgTestDataPath == NULL){
            const char*      directory=NULL;
            UResourceBundle* test =NULL;
            char* tdpath=NULL;
            const char* tdrelativepath;

#if defined (U_TOPBUILDDIR)
            tdrelativepath = "test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
            directory = U_TOPBUILDDIR;
#else
            tdrelativepath = ".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
            directory = pathToDataDirectory();
#endif

            tdpath = (char*) malloc(sizeof(char) *(( strlen(directory) * strlen(tdrelativepath)) + 100));


            /* u_getDataDirectory shoul return \source\data ... set the
            * directory to ..\source\data\..\test\testdata\out\testdata
            */
            strcpy(tdpath, directory);
            strcat(tdpath, tdrelativepath);
            strcat(tdpath,"testdata");

            test=ures_open(tdpath, "testtypes", &err);

            if(U_FAILURE(err)){
                err = U_FILE_ACCESS_ERROR;
                log_err("Could not load testtypes.res in testdata bundle with path %s - %s\n", tdpath, u_errorName(err));
                return "";
            }
            ures_close(test);
            fgTestDataPath = tdpath;
        }
        return fgTestDataPath;
    }

    virtual const char* getTestDataPath(UErrorCode& err) {
        return loadTestData(err);
    }
};

const char* DataDrivenLogger::fgDataDir = NULL;
char* DataDrivenLogger::fgTestDataPath = NULL;

static int64_t
uto64(const UChar     *buffer)
{
    int64_t result = 0;
    /* iterate through buffer */
    while(*buffer) {
        /* read the next digit */
        result *= 16;
        if (!u_isxdigit(*buffer)) {
            log_err("\\u%04X is not a valid hex digit for this test\n", (UChar)*buffer);
        }
        result += *buffer - 0x0030 - (*buffer >= 0x0041 ? (*buffer >= 0x0061 ? 39 : 7) : 0);
        buffer++;
    }
    return result;
}


U_CDECL_BEGIN
static void U_CALLCONV DataDrivenPrintf(void)
{
#if !UCONFIG_NO_FORMATTING
    UErrorCode errorCode;
    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    DataDrivenLogger logger;
    UChar uBuffer[512];
    char cBuffer[512];
    char cFormat[sizeof(cBuffer)];
    char cExpected[sizeof(cBuffer)];
    UnicodeString tempStr;
    UChar format[512];
    UChar expectedResult[512];
    UChar argument[512];
    int32_t i;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    double dbl;
    int32_t uBufferLenReturned;

    const char *fileLocale = "en_US_POSIX";
    int32_t uFileBufferLenReturned;
    UFILE *testFile;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("icuio", logger, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("printf", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    log_err("error retrieving icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
                testFile = u_fopen(STANDARD_TEST_FILE, "w", fileLocale, "UTF-8");
                if (!testFile) {
                    log_err("Can't open test file - %s\n",
                            STANDARD_TEST_FILE);
                    continue;
                }
                u_memset(uBuffer, 0x2A, sizeof(uBuffer)/sizeof(uBuffer[0]));
                uBuffer[sizeof(uBuffer)/sizeof(uBuffer[0])-1] = 0;
                tempStr=testCase->getString("format", errorCode);
                tempStr.extract(format, sizeof(format)/sizeof(format[0]), errorCode);
                tempStr=testCase->getString("result", errorCode);
                tempStr.extract(expectedResult, sizeof(expectedResult)/sizeof(expectedResult[0]), errorCode);
                tempStr=testCase->getString("argument", errorCode);
                tempStr.extract(argument, sizeof(argument)/sizeof(argument[0]), errorCode);
                u_austrncpy(cBuffer, format, sizeof(cBuffer));
                if(U_FAILURE(errorCode)) {
                    log_err("error retrieving icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
                log_verbose("Test %d: format=\"%s\"\n", i, cBuffer);
                switch (testCase->getString("argumentType", errorCode)[0]) {
                case 0x64:  // 'd' double
                    dbl = atof(u_austrcpy(cBuffer, argument));
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, dbl);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, dbl);
                    break;
                case 0x31:  // '1' int8_t
                    i8 = (int8_t)uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, i8);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, i8);
                    break;
                case 0x32:  // '2' int16_t
                    i16 = (int16_t)uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, i16);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, i16);
                    break;
                case 0x34:  // '4' int32_t
                    i32 = (int32_t)uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, i32);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, i32);
                    break;
                case 0x38:  // '8' int64_t
                    i64 = uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, i64);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, i64);
                    break;
                case 0x73:  // 's' char *
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, cBuffer);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, cBuffer);
                    break;
                case 0x53:  // 'S' UChar *
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, argument);
                    uFileBufferLenReturned = u_fprintf_u(testFile, format, argument);
                    break;
                default:
                    uBufferLenReturned = 0;
                    uFileBufferLenReturned = 0;
                    log_err("Unknown type %c for test %d\n", testCase->getString("argumentType", errorCode)[0], i);
                }
                if (u_strcmp(uBuffer, expectedResult) != 0) {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    u_austrncpy(cFormat, format, sizeof(cFormat));
                    u_austrncpy(cExpected, expectedResult, sizeof(cExpected));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE string test case %d \"%s\" - Got: \"%s\" Expected: \"%s\"\n",
                            i, cFormat, cBuffer, cExpected);
                }
                if (uBuffer[uBufferLenReturned-1] == 0
                    || uBuffer[uBufferLenReturned] != 0
                    || uBuffer[uBufferLenReturned+1] != 0x2A
                    || uBuffer[uBufferLenReturned+2] != 0x2A)
                {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE test case %d - \"%s\" wrong amount of characters was written. Got %d.\n",
                            i, cBuffer, uBufferLenReturned);
                }
                u_fclose(testFile);
                testFile = u_fopen(STANDARD_TEST_FILE, "r", fileLocale, "UTF-8");
                if (!testFile) {
                    log_err("Can't open test file - %s\n",
                            STANDARD_TEST_FILE);
                }
                uBuffer[0]=0;
                u_fgets(uBuffer, sizeof(uBuffer)/sizeof(uBuffer[0]), testFile);
                if (u_strcmp(uBuffer, expectedResult) != 0) {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    u_austrncpy(cFormat, format, sizeof(cFormat));
                    u_austrncpy(cExpected, expectedResult, sizeof(cExpected));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE file test case %d \"%s\" - Got: \"%s\" Expected: \"%s\"\n",
                            i, cFormat, cBuffer, cExpected);
                }
                if (uFileBufferLenReturned != uBufferLenReturned)
                {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE uFileBufferLenReturned(%d) != uBufferLenReturned(%d)\n",
                            uFileBufferLenReturned, uBufferLenReturned);
                }

                if(U_FAILURE(errorCode)) {
                    log_err("error running icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
                u_fclose(testFile);
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        log_err("Failed: could not load test icuio data\n");
    }
#endif
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV DataDrivenScanf(void)
{
#if !UCONFIG_NO_FORMATTING
    UErrorCode errorCode;
    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    DataDrivenLogger logger;
    UChar uBuffer[512];
    char cBuffer[512];
    char cExpected[sizeof(cBuffer)];
    UnicodeString tempStr;
    UChar format[512];
    UChar expectedResult[512];
    UChar argument[512];
    int32_t i;
    int8_t i8, expected8;
    int16_t i16, expected16;
    int32_t i32, expected32;
    int64_t i64, expected64;
    double dbl, expectedDbl;
    int32_t uBufferLenReturned;

    //const char *fileLocale = "en_US_POSIX";
    //int32_t uFileBufferLenReturned;
    //UFILE *testFile;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("icuio", logger, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("scanf", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    log_err("error retrieving icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
/*                testFile = u_fopen(STANDARD_TEST_FILE, "w", fileLocale, "UTF-8");
                if (!testFile) {
                    log_err("Can't open test file - %s\n",
                            STANDARD_TEST_FILE);
                }*/
                u_memset(uBuffer, 0x2A, sizeof(uBuffer)/sizeof(uBuffer[0]));
                uBuffer[sizeof(uBuffer)/sizeof(uBuffer[0])-1] = 0;
                tempStr=testCase->getString("format", errorCode);
                tempStr.extract(format, sizeof(format)/sizeof(format[0]), errorCode);
                tempStr=testCase->getString("result", errorCode);
                tempStr.extract(expectedResult, sizeof(expectedResult)/sizeof(expectedResult[0]), errorCode);
                tempStr=testCase->getString("argument", errorCode);
                tempStr.extract(argument, sizeof(argument)/sizeof(argument[0]), errorCode);
                u_austrncpy(cBuffer, format, sizeof(cBuffer));
                if(U_FAILURE(errorCode)) {
                    log_err("error retrieving icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
                log_verbose("Test %d: format=\"%s\"\n", i, cBuffer);
                switch (testCase->getString("argumentType", errorCode)[0]) {
                case 0x64:  // 'd' double
                    expectedDbl = atof(u_austrcpy(cBuffer, expectedResult));
                    uBufferLenReturned = u_sscanf_u(argument, format, &dbl);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, dbl);
                    if (dbl != expectedDbl) {
                        log_err("error in scanf test case[%d] Got: %f Exp: %f\n",
                                i, dbl, expectedDbl);
                    }
                    break;
                case 0x31:  // '1' int8_t
                    expected8 = (int8_t)uto64(expectedResult);
                    uBufferLenReturned = u_sscanf_u(argument, format, &i8);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, i8);
                    if (i8 != expected8) {
                        log_err("error in scanf test case[%d] Got: %02X Exp: %02X\n",
                                i, i8, expected8);
                    }
                    break;
                case 0x32:  // '2' int16_t
                    expected16 = (int16_t)uto64(expectedResult);
                    uBufferLenReturned = u_sscanf_u(argument, format, &i16);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, i16);
                    if (i16 != expected16) {
                        log_err("error in scanf test case[%d] Got: %04X Exp: %04X\n",
                                i, i16, expected16);
                    }
                    break;
                case 0x34:  // '4' int32_t
                    expected32 = (int32_t)uto64(expectedResult);
                    uBufferLenReturned = u_sscanf_u(argument, format, &i32);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, i32);
                    if (i32 != expected32) {
                        log_err("error in scanf test case[%d] Got: %08X Exp: %08X\n",
                                i, i32, expected32);
                    }
                    break;
                case 0x38:  // '8' int64_t
                    expected64 = uto64(expectedResult);
                    uBufferLenReturned = u_sscanf_u(argument, format, &i64);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, i64);
                    if (i64 != expected64) {
                        log_err("error in scanf 64-bit. Test case = %d\n", i);
                    }
                    break;
                case 0x73:  // 's' char *
                    u_austrcpy(cExpected, expectedResult);
                    uBufferLenReturned = u_sscanf_u(argument, format, cBuffer);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, cBuffer);
                    if (strcmp(cBuffer, cExpected) != 0) {
                        log_err("error in scanf char * string. Got \"%s\" Expected \"%s\". Test case = %d\n", cBuffer, cExpected, i);
                    }
                    break;
                case 0x53:  // 'S' UChar *
                    uBufferLenReturned = u_sscanf_u(argument, format, uBuffer);
                    //uFileBufferLenReturned = u_fscanf_u(testFile, format, argument);
                    if (u_strcmp(uBuffer, expectedResult) != 0) {
                        u_austrcpy(cExpected, format);
                        u_austrcpy(cBuffer, uBuffer);
                        log_err("error in scanf UChar * string %s Got: \"%s\". Test case = %d\n", cExpected, cBuffer, i);
                    }
                    break;
                default:
                    uBufferLenReturned = 0;
                    //uFileBufferLenReturned = 0;
                    log_err("Unknown type %c for test %d\n", testCase->getString("argumentType", errorCode)[0], i);
                }
                if (uBufferLenReturned != 1) {
                    log_err("error scanf converted %d arguments. Test case = %d\n", uBufferLenReturned, i);
                }
/*                if (u_strcmp(uBuffer, expectedResult) != 0) {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    u_austrncpy(cFormat, format, sizeof(cFormat));
                    u_austrncpy(cExpected, expectedResult, sizeof(cExpected));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE string test case %d \"%s\" - Got: \"%s\" Expected: \"%s\"\n",
                            i, cFormat, cBuffer, cExpected);
                }
                if (uBuffer[uBufferLenReturned-1] == 0
                    || uBuffer[uBufferLenReturned] != 0
                    || uBuffer[uBufferLenReturned+1] != 0x2A
                    || uBuffer[uBufferLenReturned+2] != 0x2A)
                {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE test case %d - \"%s\" wrong amount of characters was written. Got %d.\n",
                            i, cBuffer, uBufferLenReturned);
                }*/
/*                u_fclose(testFile);
                testFile = u_fopen(STANDARD_TEST_FILE, "r", fileLocale, "UTF-8");
                if (!testFile) {
                    log_err("Can't open test file - %s\n",
                            STANDARD_TEST_FILE);
                }
                uBuffer[0];
                u_fgets(uBuffer, sizeof(uBuffer)/sizeof(uBuffer[0]), testFile);
                if (u_strcmp(uBuffer, expectedResult) != 0) {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    u_austrncpy(cFormat, format, sizeof(cFormat));
                    u_austrncpy(cExpected, expectedResult, sizeof(cExpected));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE file test case %d \"%s\" - Got: \"%s\" Expected: \"%s\"\n",
                            i, cFormat, cBuffer, cExpected);
                }
                if (uFileBufferLenReturned != uBufferLenReturned)
                {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE uFileBufferLenReturned(%d) != uBufferLenReturned(%d)\n",
                            uFileBufferLenReturned, uBufferLenReturned);
                }
*/
                if(U_FAILURE(errorCode)) {
                    log_err("error running icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
//                u_fclose(testFile);
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        log_err("Failed: could not load test icuio data\n");
    }
#endif
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV DataDrivenPrintfPrecision(void)
{
#if !UCONFIG_NO_FORMATTING
    UErrorCode errorCode;
    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    DataDrivenLogger logger;
    UChar uBuffer[512];
    char cBuffer[512];
    char cFormat[sizeof(cBuffer)];
    char cExpected[sizeof(cBuffer)];
    UnicodeString tempStr;
    UChar format[512];
    UChar expectedResult[512];
    UChar argument[512];
    int32_t precision;
    int32_t i;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    double dbl;
    int32_t uBufferLenReturned;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("icuio", logger, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("printfPrecision", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    log_err("error retrieving icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
                u_memset(uBuffer, 0x2A, sizeof(uBuffer)/sizeof(uBuffer[0]));
                uBuffer[sizeof(uBuffer)/sizeof(uBuffer[0])-1] = 0;
                tempStr=testCase->getString("format", errorCode);
                tempStr.extract(format, sizeof(format)/sizeof(format[0]), errorCode);
                tempStr=testCase->getString("result", errorCode);
                tempStr.extract(expectedResult, sizeof(expectedResult)/sizeof(expectedResult[0]), errorCode);
                tempStr=testCase->getString("argument", errorCode);
                tempStr.extract(argument, sizeof(argument)/sizeof(argument[0]), errorCode);
                precision=testCase->getInt28("precision", errorCode);
                u_austrncpy(cBuffer, format, sizeof(cBuffer));
                if(U_FAILURE(errorCode)) {
                    log_err("error retrieving icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
                log_verbose("Test %d: format=\"%s\"\n", i, cBuffer);
                switch (testCase->getString("argumentType", errorCode)[0]) {
                case 0x64:  // 'd' double
                    dbl = atof(u_austrcpy(cBuffer, argument));
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, dbl);
                    break;
                case 0x31:  // '1' int8_t
                    i8 = (int8_t)uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, i8);
                    break;
                case 0x32:  // '2' int16_t
                    i16 = (int16_t)uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, i16);
                    break;
                case 0x34:  // '4' int32_t
                    i32 = (int32_t)uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, i32);
                    break;
                case 0x38:  // '8' int64_t
                    i64 = uto64(argument);
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, i64);
                    break;
                case 0x73:  // 's' char *
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, cBuffer);
                    break;
                case 0x53:  // 'S' UChar *
                    uBufferLenReturned = u_sprintf_u(uBuffer, format, precision, argument);
                    break;
                default:
                    uBufferLenReturned = 0;
                    log_err("Unknown type %c for test %d\n", testCase->getString("argumentType", errorCode)[0], i);
                }
                if (u_strcmp(uBuffer, expectedResult) != 0) {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    u_austrncpy(cFormat, format, sizeof(cFormat));
                    u_austrncpy(cExpected, expectedResult, sizeof(cExpected));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE test case %d \"%s\" - Got: \"%s\" Expected: \"%s\"\n",
                            i, cFormat, cBuffer, cExpected);
                }
                if (uBuffer[uBufferLenReturned-1] == 0
                    || uBuffer[uBufferLenReturned] != 0
                    || uBuffer[uBufferLenReturned+1] != 0x2A
                    || uBuffer[uBufferLenReturned+2] != 0x2A)
                {
                    u_austrncpy(cBuffer, uBuffer, sizeof(cBuffer));
                    cBuffer[sizeof(cBuffer)-1] = 0;
                    log_err("FAILURE test case %d - \"%s\" wrong amount of characters was written. Got %d.\n",
                            i, cBuffer, uBufferLenReturned);
                }
                if(U_FAILURE(errorCode)) {
                    log_err("error running icuio/printf test case %d - %s\n",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        log_err("Failed: could not load test icuio data\n");
    }
#endif
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV TestStream(void)
{
#if U_IOSTREAM_SOURCE >= 198506
    const UChar thisMu[] = { 0x74, 0x48, 0x69, 0x73, 0x3BC, 0};
    const UChar mu[] = { 0x6D, 0x75, 0};
    UnicodeString str1 = UNICODE_STRING_SIMPLE("str1");
    UnicodeString str2 = UNICODE_STRING_SIMPLE(" <<");
    UnicodeString str3 = UNICODE_STRING_SIMPLE("2");
    UnicodeString str4 = UNICODE_STRING_SIMPLE(" UTF-8 ");
    UnicodeString inStr = UNICODE_STRING_SIMPLE(" UTF-8 ");
    UnicodeString inStr2;
    char defConvName[UCNV_MAX_CONVERTER_NAME_LENGTH*2];
    char inStrC[128];
    UErrorCode status = U_ZERO_ERROR;
    UConverter *defConv;
    static const char testStr[] = "\x42\x65\x67\x69\x6E\x6E\x69\x6E\x67\x20\x6F\x66\x20\x74\x65\x73\x74\x20\x73\x74\x72\x31\x20\x20\x20\x3C\x3C\x32\x31\x20" UTF8_NEW_LINE "\x20\x55\x54\x46\x2D\x38\x20\xCE\xBC\xF0\x90\x80\x81\xF0\x90\x80\x82";

    str4.append((UChar32)0x03BC);   /* mu */
    str4.append((UChar32)0x10001);
    str4.append((UChar32)0x10002);

    /* release the default converter and use utf-8 for a bit */
    defConv = u_getDefaultConverter(&status);
    if (U_FAILURE(status)) {
        log_err("Can't get default converter\n");
        return;
    }
    ucnv_close(defConv);
    strncpy(defConvName, ucnv_getDefaultName(), sizeof(defConvName)/sizeof(defConvName[0]));
    ucnv_setDefaultName("UTF-8");

#ifdef USE_SSTREAM
    ostringstream outTestStream;
    istringstream inTestStream("\x20\x74\x48\x69\x73\xCE\xBC\xE2\x80\x82\x20\x6D\x75\x20\x77\x6F\x72\x6C\x64");
#else
    char testStreamBuf[512];
    ostrstream outTestStream(testStreamBuf, sizeof(testStreamBuf));
    istrstream inTestStream("\x20\x74\x48\x69\x73\xCE\xBC\xE2\x80\x82\x20\x6D\x75\x20\x77\x6F\x72\x6C\x64", 0);

    /* initialize testStreamBuf */
    memset(testStreamBuf, '*', sizeof(testStreamBuf));
    testStreamBuf[sizeof(testStreamBuf)-1] = 0;
#endif

    outTestStream << "\x42\x65\x67\x69\x6E\x6E\x69\x6E\x67\x20\x6F\x66\x20\x74\x65\x73\x74\x20";
    outTestStream << str1 << "\x20\x20" << str2 << str3 << "\x31\x20" << UTF8_NEW_LINE << str4 << ends;
#ifdef USE_SSTREAM
    string tempStr = outTestStream.str();
    const char *testStreamBuf = tempStr.c_str();
#endif
    if (strcmp(testStreamBuf, testStr) != 0) {
        log_err("Got: \"%s\", Expected: \"%s\"\n", testStreamBuf, testStr);
    }

    inTestStream >> inStr >> inStr2;
    if (inStr.compare(thisMu) != 0) {
        u_austrncpy(inStrC, inStr.getBuffer(), inStr.length());
        inStrC[inStr.length()] = 0;
        log_err("Got: \"%s\", Expected: \"tHis\\u03BC\"\n", inStrC);
    }
    if (inStr2.compare(mu) != 0) {
        u_austrncpy(inStrC, inStr.getBuffer(), inStr.length());
        inStrC[inStr.length()] = 0;
        log_err("Got: \"%s\", Expected: \"mu\"\n", inStrC);
    }

    /* return the default converter to the original state. */
    ucnv_setDefaultName(defConvName);
    defConv = u_getDefaultConverter(&status);
    if (U_FAILURE(status)) {
        log_err("Can't get default converter");
        return;
    }
    ucnv_close(defConv);
#else
    log_info("U_IOSTREAM_SOURCE is disabled\n");
#endif
}
U_CDECL_END

static void addAllTests(TestNode** root) {
    addFileTest(root);
    addStringTest(root);

#if !UCONFIG_NO_FORMATTING
    addTest(root, &DataDrivenPrintf, "datadriv/DataDrivenPrintf");
    addTest(root, &DataDrivenPrintfPrecision, "datadriv/DataDrivenPrintfPrecision");
    addTest(root, &DataDrivenScanf, "datadriv/DataDrivenScanf");
#endif
    addTest(root, &TestStream, "stream/TestStream");
}

/* returns the path to icu/source/data/out */
static const char *ctest_dataOutDir()
{
    static const char *dataOutDir = NULL;

    if(dataOutDir) {
        return dataOutDir;
    }

    /* U_TOPBUILDDIR is set by the makefiles on UNIXes when building cintltst and intltst
    //              to point to the top of the build hierarchy, which may or
    //              may not be the same as the source directory, depending on
    //              the configure options used.  At any rate,
    //              set the data path to the built data from this directory.
    //              The value is complete with quotes, so it can be used
    //              as-is as a string constant.
    */
#if defined (U_TOPBUILDDIR)
    {
        dataOutDir = U_TOPBUILDDIR "data"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
    }
#else

    /* On Windows, the file name obtained from __FILE__ includes a full path.
     *             This file is "wherever\icu\source\test\cintltst\cintltst.c"
     *             Change to    "wherever\icu\source\data"
     */
    {
        static char p[sizeof(__FILE__) + 20];
        char *pBackSlash;
        int i;

        strcpy(p, __FILE__);
        /* We want to back over three '\' chars.                            */
        /*   Only Windows should end up here, so looking for '\' is safe.   */
        for (i=1; i<=3; i++) {
            pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
            if (pBackSlash != NULL) {
                *pBackSlash = 0;        /* Truncate the string at the '\'   */
            }
        }

        if (pBackSlash != NULL) {
            /* We found and truncated three names from the path.
             *  Now append "source\data" and set the environment
             */
            strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING);
            dataOutDir = p;
        }
        else {
            /* __FILE__ on MSVC7 does not contain the directory */
            FILE *file = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
            if (file) {
                fclose(file);
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
            }
            else {
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
            }
        }
    }
#endif

    return dataOutDir;
}

/*  ctest_setICU_DATA  - if the ICU_DATA environment variable is not already
 *                       set, try to deduce the directory in which ICU was built,
 *                       and set ICU_DATA to "icu/source/data" in that location.
 *                       The intent is to allow the tests to have a good chance
 *                       of running without requiring that the user manually set
 *                       ICU_DATA.  Common data isn't a problem, since it is
 *                       picked up via a static (build time) reference, but the
 *                       tests dynamically load some data.
 */
static void ctest_setICU_DATA() {

    /* No location for the data dir was identifiable.
     *   Add other fallbacks for the test data location here if the need arises
     */
    if (getenv("ICU_DATA") == NULL) {
        /* If ICU_DATA isn't set, set it to the usual location */
        u_setDataDirectory(ctest_dataOutDir());
    }
}

int main(int argc, char* argv[])
{
    int32_t nerrors = 0;
    TestNode *root = NULL;
    UErrorCode errorCode = U_ZERO_ERROR;

    /* Check whether ICU will initialize without forcing the build data directory into
    *  the ICU_DATA path.  Success here means either the data dll contains data, or that
    *  this test program was run with ICU_DATA set externally.  Failure of this check
    *  is normal when ICU data is not packaged into a shared library.
    *
    *  Whether or not this test succeeds, we want to cleanup and reinitialize
    *  with a data path so that data loading from individual files can be tested.
    */
    u_init(&errorCode);
    if (U_FAILURE(errorCode)) {
        fprintf(stderr,
            "#### Note:  ICU Init without build-specific setDataDirectory() failed.\n");
    }
    u_cleanup();
    errorCode = U_ZERO_ERROR;

    /* Initialize ICU */
    ctest_setICU_DATA();    /* u_setDataDirectory() must happen Before u_init() */
    u_init(&errorCode);
    if (U_FAILURE(errorCode)) {
        fprintf(stderr,
            "#### ERROR! %s: u_init() failed with status = \"%s\".\n" 
            "*** Check the ICU_DATA environment variable and \n"
            "*** check that the data files are present.\n", argv[0], u_errorName(errorCode));
        return 1;
    }

    fprintf(stdout, "Default charset for this run is %s\n", ucnv_getDefaultName());

    addAllTests(&root);
    nerrors = processArgs(root, argc, argv);

#if 1
    {
        FILE* fileToRemove = fopen(STANDARD_TEST_FILE, "r");
        /* This should delete any temporary files. */
        if (fileToRemove) {
            fclose(fileToRemove);
            if (remove(STANDARD_TEST_FILE) != 0) {
                /* Maybe someone didn't close the file correctly. */
                fprintf(stderr, "FAIL: Could not delete %s\n", STANDARD_TEST_FILE);
                nerrors += 1;
            }
        }
    }
#endif

    cleanUpTestTree(root);
    DataDrivenLogger::cleanUp();
    u_cleanup();
    return nerrors;
}
