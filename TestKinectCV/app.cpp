#include "App.h"	
#include "synthTone.h"

void App::Init()
{
	// Put initialization stuff here
	

	SDL_Init(SDL_INIT_AUDIO);

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
		
		depthBuffer = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];

		while(!getFrame());
		config = new Configure(Rect(30,30,30,30),Point(0,0),Point(0,0));
		Mat mat(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, &depthBuffer[0]);
		Mat NormMat;
		Mat DisplayMat;
		Mat imgCropped;
		cv::normalize(mat, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
		NormMat.convertTo(DisplayMat, CV_8UC3);
		//flipAndDisplay(DisplayMat, "CvOutput", 0);
		config->cropWindow(DisplayMat);
		//config->showImage(DisplayMat, config->cropRect);
		//cv::namedWindow("CvOutput1", CV_WINDOW_KEEPRATIO);
		//flipAndDisplay(DisplayMat, "CvOutput1", 0);
		SafeRelease(depthFrame);
		cv::namedWindow("CvOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
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
		getFrame();
		cv::Mat mat(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, &depthBuffer[0]);		
		for (int j = 0; j < mat.rows; j++)
		{
			for (int i = 0; i < mat.cols; i++)
			{
				if (mat.at<uint16>(j, i) > 1400)
				{
					mat.at<uint16>(j, i) = 0;
				}
			}
		}
		//printf("Pixel val: %d \n", mat.at<uint16>(200,200));
		Mat matCropped = mat.clone();
		medianBlur(mat.clone(), matCropped, 5);
		config->showImage(matCropped, config->cropRect);
		double min, max;
		double vmin, vmax;
		int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };

		minMaxIdx(matCropped, &vmin, &vmax, idx_min, idx_max);
		printf("MinVal: %f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MayY %d   \n"
			, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);


		cv::Mat NormMat;
		cv::Mat FlippedMat;
		cv::Mat ScalledMat;
		cv::Mat BeforeColouredMat;
		cv::Mat BeforeInvertedMat;
		cv::Mat ThresholdedMat;
		Mat PlottedMat;
		//cv::threshold(mat, ThresholdedMat, 255.0, 0 ,THRESH_TOZERO);
		cv::normalize(mat, NormMat, 0, 255, CV_MINMAX, CV_16UC1);

		int colmap = 4;
		cv::Mat DisplayMat;
		NormMat.convertTo(BeforeColouredMat, CV_8UC3);
		cv::applyColorMap(BeforeColouredMat, BeforeInvertedMat, colmap);
		//cv::applyColorMap(BeforeColouredMat, DisplayMat, colmap);
		cv::bitwise_not(BeforeInvertedMat, DisplayMat);
		//circle(DisplayMat, Point(maxLocation2,maxLocation2), 30, Scalar(0, 0, 255), 2);
		//printf("Value: %d \n", mat.at<uint16>(510,420));
		config->showImage(DisplayMat, config->cropRect);
		circle(DisplayMat, Point(idx_min[1],idx_min[0]), 30, Scalar(0, 255, 0), 2);
		circle(DisplayMat, Point(idx_max[1], idx_max[0]), 30, Scalar(0, 0, 255), 2);
		

		/*int duration = 1000;
		double Hz = 440;

		Beeper b;
		b.beep(Hz, duration);
		b.wait();*/
		
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