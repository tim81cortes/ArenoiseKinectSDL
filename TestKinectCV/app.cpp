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
	}

	

}
bool App::getSensorPresence()
{
	return foundSensor;
}
void App::Tick(float deltaTime)
{
	//cv::setBreakOnError(true);
	
	//put update and drawing stuff here
	HRESULT hr;
	
	
	
	
	if (true == foundSensor)
	{
		hr = m_depthFrameReader->AcquireLatestFrame(&depthFrame);
		if (FAILED(hr))
		{
			return;
		}
	
		//printf("Copying data \n");

		hr = depthFrame->CopyFrameDataToArray(DEPTHMAPWIDTH * DEPTHMAPHEIGHT, depthBuffer);	
		if (FAILED(hr))
		{	
			printf("There was a problem copying the frame data to a buffer. \n");
			return;
		}

	}

		cv::Mat mat(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, &depthBuffer[0]);
		//printf("Depth Buffer Size %d", sizeof(depthBuffer));
		double scale_factor = 1.0 / 256;
		cv::Mat NormMat;
		cv::Mat FlippedMat;
		cv::Mat ScalledMat;
		cv::Mat BeforeColouredMat;
		cv::Mat BeforeInvertedMat;
		cv::flip(mat, FlippedMat, 1);
		
		cv::normalize(FlippedMat, NormMat, 0, 255, CV_MINMAX, CV_16UC1);

		
		cv::Mat DisplayMat;
		NormMat.convertTo(BeforeColouredMat, CV_8UC3);
		cv::applyColorMap(BeforeColouredMat, BeforeInvertedMat, 4);
		cv::bitwise_not(BeforeInvertedMat, DisplayMat);
		cv::imshow("CvOutput", DisplayMat);	
		cv::waitKey(10);
		//printf("Value: %d \n", mat.at<uint16>(510,420));

	//double iterRank = 0;
	//uint32 iterRankIndex = 0;
	//uint16 iterFile = 0;
	//uint16 depthFlip = 0;

	//Copy depth data to the screen.
		//for (int i = 0; i < DEPTHMAPWIDTH * DEPTHMAPHEIGHT; i++)
		//{
		//uint16 depth = depthBuffer[i];
		////	iterRank = trunc(i / DEPTHMAPWIDTH);
		////	iterRankIndex = uint32(iterRank * DEPTHMAPWIDTH);
		////	iterFile = i - iterRankIndex;
		////
		////
		////	if (foundSensor)
		////	{
		////		depthFlip = depthBuffer[iterRankIndex + DEPTHMAPWIDTH - iterFile];
		////	}
		////	else
		////	{
		////		depthFlip = 1000;
		////	}
		////	//std::cout << "Depth:" << i << " IterRank: " << iterRank << " IterRankIndex" << iterRankIndex << " Iterfile" << iterFile << " Depthflip: " << depthFlip << '\n';
		////
		//	m_pixelBuffer[i] = depth;

		//}

	/*for (int i = 0; i < mat.rows/4; i++) {
		for (int j = 0; j < mat.cols/4; j++)
		{
			if (i % 20 == 0)
			{
				printf("Row: %d Column: %d Value: %d \n", i, j, mat.at<double>(i,j));
			}
			
		}
		

	}*/
	//printf("Value: %d \n", NormMat.at<uint16>(20, 20));

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
	cvDestroyWindow("CvOutput");
	free(depthBuffer);
}