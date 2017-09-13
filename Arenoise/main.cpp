// File: main.cpp
// Name: Tim Aylott.cpp
// Date: 01/09/2017
// Course: MSc Software Systems
// Description: Arenoise is a Digital Musical Instrument which 
// uses sand in a sandbox as an interaction meduim. This is 
// component 1 which reads the sensor data, processes interaction events, 
// sends them to component 2 and then displays visual feedback. 
// This file controls the lifecycle of the app and the timestep of the main loop.

#include <iostream>
#include <chrono>
#include <string>
#include "App.h"

using namespace std::chrono;
typedef steady_clock Clock;

#undef main
int main(int, char**)
{
	// Depth frame acquisition, loop time step and memory and lifecycle management used 
	// with permission from Max Oomen. See my masters thesis for referrence.
	
	////allocate a pixel buffer
	uint32* pixelBuffer = new uint32[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];
	if (pixelBuffer == nullptr)
		return 4;

	//printf("Pixel buffer allocated.\n");

	////clear the pixel buffer
	memset(pixelBuffer, 0, DEPTHMAPWIDTH * DEPTHMAPHEIGHT * 4);

	UdpTransmitSocket transmitSocket(IpEndpointName(ADDRESS, PORT));
	

	char buffer[OUTPUT_BUFFER_SIZE];
	
	// Configure OSC packet stream
	osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

	App app;
	app.SetPixelBuffer(pixelBuffer);
	app.Init();
	
	auto lastTime = Clock::now();

	bool running = true;
	while (running)
	{
		//calculate delta time
		const auto now = Clock::now();
		const auto duration = duration_cast<microseconds>(now - lastTime);
		const float deltaTime = duration.count() / 1000000.0f;
		lastTime = now;
		
		//update the application
		app.Tick(deltaTime, p, transmitSocket);
	}
	
	//clean up
	app.Shutdown();
	return 0;
}

