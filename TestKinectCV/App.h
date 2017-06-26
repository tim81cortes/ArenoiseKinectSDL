
#pragma once
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

// some often used STL Header Files
#include <iostream>
#include <vector>
#include <memory>
#include <Kinect.h>

// size of window
#define DEPTHMAPWIDTH 512
#define DEPTHMAPHEIGHT 424

#define SCREENWIDTH 1024
#define SCREENHEIGHT 768

// some useful typedefs for explicit type sizes
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

// safe way of deflating a COM object
template<typename T>
void SafeRelease(T& ptr) { if(ptr) { ptr->Release(); ptr = nullptr; } }

class App {
public:
	void Init();
	void Tick(float deltaTime);
	void Shutdown();

	void SetPixelBuffer(uint32* pixelBuffer) {m_pixelBuffer = pixelBuffer; }

	// safe way of plotting a pixel
	void Plot(int x, int y, uint32 color) {}

	bool getSensorPresence();

private:
	//pointer to buffer that containes pixels that get pushed to the screen
	// size of the buffer is SCRWIDTH * SCRHIGHT * sizeof(uint32)

	uint32* m_pixelBuffer = nullptr;
	IKinectSensor* m_sensor = nullptr;
	IDepthFrameReader* m_depthFrameReader = nullptr;
	uint16* depthBuffer = nullptr;
	bool foundSensor = false;
};