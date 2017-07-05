#include "App.h"

void App::Init()
{
	// Put initialization stuff here
	HRESULT hr;
	BOOLEAN *kinectAvailability = false;
	hr = GetDefaultKinectSensor(&m_sensor);
	m_sensor->get_IsAvailable(kinectAvailability);
	if (FAILED(hr))
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
		cv::namedWindow("CvOutput", CV_WINDOW_KEEPRATIO);
		depthBuffer = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];

		while(!getFrame());

		Mat mat(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, &depthBuffer[0]);
		Mat NormMat;
		Mat DisplayMat;
		cv::normalize(mat, NormMat, 0, 255, CV_MINMAX, CV_16UC1);

		NormMat.convertTo(DisplayMat, CV_8UC3);
		flipAndDisplay(DisplayMat, "CvOutput", 0);
		SafeRelease(depthFrame);
		
	}

	

}
void App::flipAndDisplay(Mat& toFlip, const String window, int wait)
{
	cv::Mat toShow;
	cv::flip(toFlip, toShow, 1);
	cv::imshow("CvOutput", toShow);
	cv::waitKey(wait);
}
bool App::getFrame()
{
	HRESULT hr;
	hr = m_depthFrameReader->AcquireLatestFrame(&depthFrame);
	if (FAILED(hr))
	{
		return false;
	}

	//printf("Copying data \n");

	hr = depthFrame->CopyFrameDataToArray(DEPTHMAPWIDTH * DEPTHMAPHEIGHT, depthBuffer);
	if (FAILED(hr))
	{
		printf("There was a problem copying the frame data to a buffer. \n");
		return false;
	}
	return true;
}
bool App::getSensorPresence()
{
	return foundSensor;
}
void App::Tick(float deltaTime)
{
	//cv::setBreakOnError(true);
	
	//put update and drawing stuff here
	//HRESULT hr;
	
	
	
	
	if (true == foundSensor)
	{
		//hr = m_depthFrameReader->AcquireLatestFrame(&depthFrame);
		//if (FAILED(hr))
		//{
		//	return;
		//}
	
		////printf("Copying data \n");

		//hr = depthFrame->CopyFrameDataToArray(DEPTHMAPWIDTH * DEPTHMAPHEIGHT, depthBuffer);	
		//if (FAILED(hr))
		//{	
		//	printf("There was a problem copying the frame data to a buffer. \n");
		//	return;
		//}
		getFrame();
	

		cv::Mat mat(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, &depthBuffer[0]);
		//printf("Depth Buffer Size %d", sizeof(depthBuffer));
		double scale_factor = 1.0 / 256;
		cv::Mat NormMat;
		cv::Mat FlippedMat;
		cv::Mat ScalledMat;
		cv::Mat BeforeColouredMat;
		cv::Mat BeforeInvertedMat;
		
		
		cv::normalize(mat, NormMat, 0, 255, CV_MINMAX, CV_16UC1);

		
		cv::Mat DisplayMat;
		NormMat.convertTo(BeforeColouredMat, CV_8UC3);
		cv::applyColorMap(BeforeColouredMat, BeforeInvertedMat, 4);
		cv::bitwise_not(BeforeInvertedMat, DisplayMat);

		//printf("Value: %d \n", mat.at<uint16>(510,420));
		flipAndDisplay(DisplayMat, "CvOutput", 2);

		SafeRelease(depthFrame);
	}

}

void App::Shutdown()
{
	// put cleaning up stuff here.
	delete[] depthBuffer;
	SafeRelease(m_sensor);
	SafeRelease(m_depthFrameReader);
	cvDestroyWindow("CvOutput");
	free(depthBuffer);
}