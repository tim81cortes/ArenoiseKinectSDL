
#pragma once
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

// some often used STL Header Files
#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <Kinect.h>
#include <string>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/photo/photo.hpp>
#include<conio.h>           
#include "configureApp.h"
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "DepthEvent.h"



#define ADDRESS "127.0.0.1"
#define PORT 8000

#define OUTPUT_BUFFER_SIZE 1024


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
	void Tick(float deltaTime, osc::OutboundPacketStream &outBoundPS, UdpTransmitSocket &trnsmtSock);
	void Shutdown();

	void SetPixelBuffer(uint32* pixelBuffer) {m_pixelBuffer = pixelBuffer; }

	// safe way of plotting a pixel
	void Plot(int x, int y, uint32 color) {}
	void flipAndDisplay(Mat & toFlip, const String window, int wait);
	bool getFrame();
	bool getSensorPresence();
	void drawAxis(Mat& img, Point p, Point q, Scalar colour, const float scale = 0.2);
	double getOrientation(const std::vector<Point> &pts, Mat &img);

	//osc::OutboundPacketStream p1;

private:
	//pointer to buffer that containes pixels that get pushed to the screen
	// size of the buffer is SCRWIDTH * SCRHIGHT * sizeof(uint32)
	double currentSideControlCoords[3] = {0,0,0}; // Knowledge
	uint32* m_pixelBuffer = nullptr;
	IKinectSensor* m_sensor = nullptr;
	IDepthFrameReader* m_depthFrameReader = nullptr;
	uint16* depthBufferUpdatedSurface = nullptr;
	uint16* depthBufferCurrentDepthFrame = nullptr;
	uint16* depthBufferOpCv = nullptr;
	uint* depthBufferOpCvSize = nullptr;
	std::queue<DepthEvent> dpthEvntQ;
	bool foundSensor = false;
	IDepthFrame* depthFrame;
	Configure* config; 
	uint16 currentMax; // Knowledge
	Mat previousSurface; // Knowledge
	Mat currentDifferenceMap; // Knowledge
	Mat updatedSurface; // Knowledge
	std::vector<Vec4i> hierarchy;
	std::vector<std::vector<Point> > contours;

	bool initFrameDone = false;
	uint16 emptyBoxMinReferrence; // Config
	uint16 _2ndInteractnAreaMinReferrence; // Config
	std::queue<Mat> framesForComparison;

	
};