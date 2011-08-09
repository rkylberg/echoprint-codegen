/*
 *  GetPCMFromFile.cpp
 *
 *  Created by Brian Whitman on 7/10/10.
 *  Copyright 2010 The Echo Nest. All rights reserved.
 *
 */
#include <AudioToolbox/AudioToolbox.h>
#include "CAStreamBasicDescription.h"
#include "Codegen_wrapper.h"

extern "C" {

extern void NSLog(CFStringRef format, ...); 

const char* GetPCMFromFile(char* filename) {
  // refactored based on gist: https://gist.github.com/1073528
  // ref: https://groups.google.com/d/topic/echoprint/oNkeS0UbEqI/discussion
  
	// open the input mp3 file as an ExtAudioFile for reading and converting:
	CFURLRef inputFileURL = CFURLCreateFromFileSystemRepresentation(NULL,(const UInt8*)filename, strlen(filename), false);
	ExtAudioFileRef inputFileRef;
	int err = ExtAudioFileOpenURL(inputFileURL, &inputFileRef);
	if (err) {
    NSLog(CFSTR("ERROR: Failed to open audio input file"));
    return nil;
  }
  
	// setup an mono LPCM format description for conversion & set as the input file's client format
	Float64 sampleRate = 11025;
	CAStreamBasicDescription outputFormat;
	outputFormat.mSampleRate = sampleRate;
	outputFormat.mFormatID = kAudioFormatLinearPCM;
	outputFormat.mChannelsPerFrame = 1;
	outputFormat.mBitsPerChannel = 32;
	outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame = 4 * outputFormat.mChannelsPerFrame;
	outputFormat.mFramesPerPacket = 1;
	outputFormat.mFormatFlags =  kAudioFormatFlagsNativeFloatPacked;// | kAudioFormatFlagIsNonInterleaved;
	
	err = ExtAudioFileSetProperty(inputFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(outputFormat), &outputFormat);
	if (err) {
    NSLog(CFSTR("ERROR: On set format %d"), err);
    return nil;
  }
  
	// read the first 30 seconds of the file into a buffer
	int secondsToDecode = 30;
	UInt32 lpcm30SecondsBufferSize = sizeof(Float32) * sampleRate * secondsToDecode; // for mono, multi channel would require * ChannelsPerFrame
	Float32* lpcm30SecondsBuffer = (Float32 *)malloc(lpcm30SecondsBufferSize);
  
	AudioBufferList audioBufferList;
	audioBufferList.mNumberBuffers = 1;
	audioBufferList.mBuffers[0].mNumberChannels = 1;
	audioBufferList.mBuffers[0].mDataByteSize = lpcm30SecondsBufferSize;
	audioBufferList.mBuffers[0].mData = lpcm30SecondsBuffer;
	
	UInt32 numberOfFrames = sampleRate * secondsToDecode; 
	NSLog(CFSTR("INFO: Expect to read %d frames"), numberOfFrames);
  err = ExtAudioFileRead(inputFileRef, &numberOfFrames, &audioBufferList);
  if (err) {
    NSLog(CFSTR("ERROR: File read conversion failed"));
    return nil;
  }
	NSLog(CFSTR("INFO: Actually read %d frames"), numberOfFrames);

	const char* fingerprint = "";
  NSLog(CFSTR("Doing codegen on %d samples..."), numberOfFrames);
  fingerprint = codegen_wrapper(lpcm30SecondsBuffer,	numberOfFrames);
  NSLog(CFSTR("Done with codegen"));
  
	free(lpcm30SecondsBuffer);
	ExtAudioFileDispose(inputFileRef);

	return fingerprint;
}


}