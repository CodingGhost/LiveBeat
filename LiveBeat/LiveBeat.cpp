#include <SDL.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>
#include "BTrack.h"
//Required libraries: BTrack+kissfft & SDL2 & libsamplerate

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;


const int MAX_RECORDING_DEVICES = 10;

//Maximum recording time
const int MAX_RECORDING_SECONDS = 30;

//Maximum recording time plus padding
const int RECORDING_BUFFER_SECONDS = MAX_RECORDING_SECONDS + 1;

//Recieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;

//Recording data buffer
Uint8* gRecordingBuffer = NULL;

//Size of data buffer
Uint32 gBufferByteSize = 0;

//Position in data buffer
Uint32 gBufferBytePosition = 0;

//Maximum position in data buffer for recording
Uint32 gBufferByteMaxPosition = 0;

int gRecordingDeviceCount = 0;

char y;

BTrack beatTracker(512);;

void audioRecordingCallback(void* userdata, Uint8* stream, int len)
{
	float* buffer = (float*)stream;
	int buflen = len / 4;
	//Copy audio from stream
	//std::memcpy(&gRecordingBuffer[gBufferBytePosition], stream, len);
	//Move along buffer
	//gBufferBytePosition += len;
	//std::cout << stream[100] << std::endl;
	beatTracker.processAudioFrame((double*)buffer);
	auto tempo = beatTracker.getCurrentTempoEstimate();

	std::cout << tempo << std::endl;
	if (beatTracker.beatDueInCurrentFrame())
	{
		std::cout << "BEAT!" << std::endl;
	}
}


int main(int argc, char* args[])
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	SDL_AudioDeviceID recordingDeviceId = 0;

	SDL_AudioSpec desiredRecordingSpec;

	SDL_zero(desiredRecordingSpec);
	desiredRecordingSpec.freq = 44100;
	desiredRecordingSpec.format = AUDIO_F32SYS;
	desiredRecordingSpec.channels = 1;
	desiredRecordingSpec.samples = 1024;
	desiredRecordingSpec.callback = audioRecordingCallback;

	gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);

	if (gRecordingDeviceCount < 1)
	{
		printf("Unable to get audio capture device! SDL Error: %s\n", SDL_GetError());
		return 0;
	}

	int index;

	for (int i = 0; i < gRecordingDeviceCount; ++i)
	{
		//Get capture device name
		const char* deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);

		printf("%d - %s\n", i, deviceName);
	}

	printf("Choose audio\n");
	scanf_s("%d", &index);

	//Open recording device
	recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, 0);

	// Device failed to open
	if (recordingDeviceId == 0)
	{
		//Report error
		printf("Failed to open recording device! SDL Error: %s", SDL_GetError());
		return 1;
	}



	//Calculate per sample bytes
	int bytesPerSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

	//Calculate bytes per second
	int bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;

	//Calculate buffer size
	gBufferByteSize = RECORDING_BUFFER_SECONDS * bytesPerSecond;

	//Calculate max buffer use
	gBufferByteMaxPosition = MAX_RECORDING_SECONDS * bytesPerSecond;

	//Allocate and initialize byte buffer
	gRecordingBuffer = new Uint8[gBufferByteSize];
	std::memset(gRecordingBuffer, 0, gBufferByteSize);


	std::printf("Recording\n");

	// begin recording
	//Go back to beginning of buffer
	gBufferBytePosition = 0;

	SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);
	while (true)
	{
		//Lock callback
		SDL_LockAudioDevice(recordingDeviceId);
		//Finished recording
		if (gBufferBytePosition > gBufferByteMaxPosition)
		{
			//Stop recording audio
			SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);
			SDL_UnlockAudioDevice(recordingDeviceId);
			break;
		}
		//Unlock callback
		SDL_UnlockAudioDevice(recordingDeviceId);
	}


	if (gRecordingBuffer != NULL)
	{
		delete[] gRecordingBuffer;
		gRecordingBuffer = NULL;
	}
	SDL_Quit();

	return 0;
}
