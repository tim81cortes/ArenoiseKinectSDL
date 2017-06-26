#include "App.h"

void App::Init()
{
	// Put initialization stuff here
	HRESULT hr;
	BOOLEAN *kinectAvailability = false;
	hr = GetDefaultKinectSensor(&m_sensor);
	m_sensor->get_IsAvailable(kinectAvailability);
	if (FAILED(hr) || !kinectAvailability)
	{
		printf("Failed to find correct sensor.\n");
		foundSensor = false;
	}
	else 
	{
		
	
		m_sensor->Open();

		IDepthFrameSource* depthFrameSource;
		hr = m_sensor->get_DepthFrameSource(&depthFrameSource);
		if (FAILED(hr))
		{
			printf("Failed to get the depthframe source.\n");
			exit(10);
		}
	
		hr = depthFrameSource->OpenReader(&m_depthFrameReader);
		if (FAILED(hr))
		{
			printf("Failed to get the depthframe reader.\n");
			exit(10);
		}
		SafeRelease(depthFrameSource);
		foundSensor = true;
	}



}
bool App::getSensorPresence()
{
	return foundSensor;
}
void App::Tick(float deltaTime)
{
	//put update and drawing stuff here
	cv::Mat A;
	HRESULT hr;
	IDepthFrame* depthFrame;
	
	depthBuffer = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];
	
	if (true == foundSensor)
	{
		hr = m_depthFrameReader->AcquireLatestFrame(&depthFrame);
		if (FAILED(hr))
		{
			return;
		}
	
		printf("Copying data \n");

		hr = depthFrame->CopyFrameDataToArray(DEPTHMAPWIDTH * DEPTHMAPHEIGHT, depthBuffer);	
		if (FAILED(hr))
		{	
			printf("There was a problem copying the frame data to a buffer. \n");
			return;
		}
	}

	double iterRank = 0;
	uint32 iterRankIndex = 0;
	uint16 iterFile = 0;
	uint16 depthFlip = 0;

	//Copy depth data to the screen.
	for (int i = 0; i < DEPTHMAPWIDTH * DEPTHMAPHEIGHT; i++)
	{
		//uint16 depth = depthBuffer[i];
		iterRank = trunc(i / DEPTHMAPWIDTH);
		iterRankIndex = uint32(iterRank * DEPTHMAPWIDTH);
		iterFile = i - iterRankIndex;
		
		
		if (foundSensor)
		{
			depthFlip = depthBuffer[iterRankIndex + DEPTHMAPWIDTH - iterFile];
		}
		else
		{
			depthFlip = 1000;
		}
		//std::cout << "Depth:" << i << " IterRank: " << iterRank << " IterRankIndex" << iterRankIndex << " Iterfile" << iterFile << " Depthflip: " << depthFlip << '\n';
		
		m_pixelBuffer[i] = depthFlip;

	}

	if (foundSensor == true)
	{
		SafeRelease(depthFrame);
	}

}

void App::Shutdown()
{
	// put cleaning up stuff here.
	delete[] depthBuffer;
	SafeRelease(m_sensor);
	SafeRelease(m_depthFrameReader);
}