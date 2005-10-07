/*
 ------------------------------------------------------------------------
Copyright (C) 2002-2004 Keith Stribley

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: TecKitJni.cpp
Responsibility: Keith Stribley
Last reviewed: Not yet.

Description:
    Implements a JNI interface to the TECkit conversion engine.
-------------------------------------------------------------------------*/

#include "org_sil_scripts_teckit_TecKitJni.h"

#include <jni.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#ifdef WIN32
#include <iostream>
#else
#include <string.h>
#endif

#include "TecKitJni.h"

using namespace std;

class TecKitJni;



//static TecKitJni * instance = NULL;

TecKitJni::TecKitJni()
: converter(NULL), mapBuffer(NULL), outputLength(0), maxOutputLength(8192), 
  maxInputLength(1024)
{
    outputBuffer = new Byte[maxOutputLength];
    inputBuffer = new char[maxInputLength];
    
    assert(outputBuffer);
}

TecKitJni::~TecKitJni()
{
    TECkit_DisposeConverter(converter);
    if (outputBuffer) delete [] outputBuffer;
    if (inputBuffer) delete [] inputBuffer;
    if (mapBuffer) delete [] mapBuffer;
}


bool TecKitJni::openMapping(const char* fileName, bool toUni)
{
    UInt16 sourceForm;
    UInt16 targetForm;
    if (toUni) // backward so reverse parameters
    {
        sourceForm = kForm_Bytes;
        targetForm = kForm_UTF8;
    }
    else
    {   
        sourceForm = kForm_UTF8;
        targetForm = kForm_Bytes;
    }
    openMapping(fileName, toUni, sourceForm, targetForm);
}


bool TecKitJni::openMapping(const char* fileName, bool toUni, UInt16 sourceForm, UInt16 targetForm)
{
    bool success = false;
    // setup for forward
    Byte mapForward = 1;
    toUnicode = toUni;
    if (!toUnicode) // backward so reverse parameters
    {
       mapForward = 0;   
    }
    
    if (fileName)
    {
        struct stat mapFileStats;
        if (stat(fileName, &mapFileStats)) 
        {
            fprintf(stderr,"TecKitJni: FAILED to stat file %s\n",fileName);
            return success;
        }
        FILE * mapFile = fopen(fileName, "rb");        
        if (mapFile)
        {
            mapBuffer = new Byte[mapFileStats.st_size];
            assert(mapBuffer);
            Byte * pBuffer = mapBuffer;
            int read = 0;
            do {
                read = fread(pBuffer,1,1024,mapFile);
                pBuffer += read;
            } while (read > 0);
            assert((pBuffer - mapBuffer) == mapFileStats.st_size);
            fclose(mapFile);
            TECkit_Status status = TECkit_CreateConverter(mapBuffer, mapFileStats.st_size, 
                mapForward, sourceForm, targetForm, &converter);
            if (status == kStatus_NoError) success = true;
            else fprintf(stderr,"TecKitJni:Error reading %s: %d\n",fileName,status);
        }
        else
        {
            fprintf(stderr,"TecKitJni:Failed to open %s\n",fileName);
        }
    }
    else
    {
        fprintf(stderr,"TecKitJni: no map file specified\n");
    }
    return success;
}


char * TecKitJni::convert(const char* input)
{
  UInt32 inLength = strlen(input);
  return convert(input, inLength);
}

char * TecKitJni::convert(const char* input, UInt32 inLength)
{

    Byte isComplete = 1; // for the moment always process at once
    TECkit_Status status;
    UInt32 inUsed = 0;
    //fprintf(stderr,"TecKitJni starting conversion %s %d\n",input, inLength);
    const Byte * byteInputBuffer = reinterpret_cast<const Byte*>(input);
    
    //fprintf(stderr,"TecKitJni starting conversion %s\n",input);
    while (((status = TECkit_ConvertBuffer(converter, byteInputBuffer, 
                                           inLength, 0, outputBuffer, 
                                           maxOutputLength, &outputLength, isComplete))
            == kStatus_OutputBufferFull) ||
           (outputLength == maxOutputLength))
    {
        fprintf(stderr,"TecKitJni:Doubling output buffer (%d too small)\n",
            maxOutputLength);
        delete [] outputBuffer;
        maxOutputLength *= 2;
	outputBuffer = new Byte[maxOutputLength];
        if (!outputBuffer) break; // memory error
    }
    //fprintf(stderr,"TecKitJni resetting, processed %d \n",outputLength);
    if (status == kStatus_NoError)
    {
        status = TECkit_ResetConverter((TECkit_Converter)converter);
        // add null terminator
        assert(outputLength<maxOutputLength);
        outputBuffer[outputLength] = '\0';
    }
    else
    {
        fprintf(stderr,"TecKitJni:Error converting data %d\n",status);
    }
    if (status == kStatus_NoError)
    {
        return reinterpret_cast<char*>(outputBuffer);
    }
    else return NULL;
}

void TecKitJni::flush()
{
    if (converter) 
    {
        if (TECkit_Flush(converter,outputBuffer,maxOutputLength, &outputLength) 
            != kStatus_NoError)
        {
            fprintf(stderr,"TecKitJni:Error flushing buffer\n");
        }
    }
}



JNIEXPORT jlong JNICALL Java_org_sil_scripts_teckit_TecKitJni_createConverter
  (JNIEnv * env, jobject, jstring path, jboolean toUnicode)
  {
      //fprintf(stderr,"in createConverter\n");
	
      jlong success = 0;
      
      TecKitJni * instance = new TecKitJni();
      const char *str = env->GetStringUTFChars(path, 0);
      success = instance->openMapping(str, toUnicode);
      env->ReleaseStringUTFChars(path, str);
      success = reinterpret_cast<jlong>(instance);
      return success;
  }
  


/*
 * Class:     DocCharConvert_Converter_TecKitJni
 * Method:    convert
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_sil_scripts_teckit_TecKitJni_convert
  (JNIEnv *env, jobject, jlong instanceId, jbyteArray iArray)
{
  jsize inLength = env->GetArrayLength(iArray);
  jbyte * inData = env->GetByteArrayElements(iArray, 0);
  TecKitJni * instance = reinterpret_cast<TecKitJni*>(instanceId);
  char * converted = instance->convert(reinterpret_cast<const char *>(inData), inLength);
  //fprintf(stderr, "Input length %d\n",inLength);
  env->ReleaseByteArrayElements(iArray, inData, 0);
  jsize outLength = instance->getOutputLength(); 
  if (outLength < 0) 
  {
    fprintf(stderr, "Invalid output length %d\n",outLength);
    outLength = 0; 
  }
  jbyteArray oArray = env->NewByteArray(outLength);
  jbyte * outData = env->GetByteArrayElements(oArray, 0);
  memcpy(outData, converted, outLength);
  env->ReleaseByteArrayElements(oArray, outData, 0);
  //fprintf(stderr, "Output length %d\n",outLength);
  return oArray;
}
  
/*
 * Class:     DocCharConvert_0002fTecKitJni
 * Method:    flush
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_sil_scripts_teckit_TecKitJni_flush
  (JNIEnv *env, jobject, jlong instanceId)
  {
      TecKitJni * instance = reinterpret_cast<TecKitJni*>(instanceId);
      if (instance) instance->flush();
  }

/*
 * Class:     DocCharConvert_0002fTecKitJni
 * Method:    destroyConverter
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_sil_scripts_teckit_TecKitJni_destroyConverter
  (JNIEnv *env, jobject, jlong instanceId)
  {
      TecKitJni * instance = reinterpret_cast<TecKitJni*>(instanceId);
      if (instance != NULL) 
      {
          delete instance;
          instance = NULL;
      }
  }

