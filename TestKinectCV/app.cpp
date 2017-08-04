#include "App.h"	
#include <algorithm>

void App::Init()
{	
	config = new Configure(Rect(30, 30, 30, 30), Point(0, 0), Point(0, 0));
	String filename = "EmptySandboxInteractionArea1.xml";
	emptyBoxMinReferrence = config->getZeroReferenceFromFile(filename);
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
		
		depthBufferCurrentDepthFrame = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];

		while(!getFrame());
		
		Mat mat2(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);
		Mat NormMat;
		Mat DisplayMat;
		Mat imgCropped;
		cv::normalize(mat2, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
		NormMat.convertTo(DisplayMat, CV_8UC3);
		config->defineRegions(DisplayMat);		
		_2ndInteractnAreaMinReferrence = config->getZeroReferenceFromMatrix(mat2);

		// Set flag to say that subsequent frames should be sent to different array
		initFrameDone = true;
		mat2.copyTo(updatedSurface);
		SafeRelease(depthFrame);
		cv::namedWindow("CvOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		cv::namedWindow("CvOutput2", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput2", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	}
}
void App::flipAndDisplay(Mat& toFlip, const String window, int wait)
{
	cv::Mat toShow;
	cv::flip(toFlip, toShow, 1);
	cv::imshow(window, toShow);
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
	// control which array gets the data
	hr = depthFrame->CopyFrameDataToArray(DEPTHMAPWIDTH * DEPTHMAPHEIGHT, depthBufferCurrentDepthFrame);
		if (FAILED(hr))
	{
		printf("There was a problem copying the frame data to a buffer. \n");
		return false;
	}
	return true;
}
bool App::getSensorPresence()
{
	// TODO find an accurate way to get sensor presence.
	return foundSensor;
}
void App::Tick(float deltaTime, osc::OutboundPacketStream &outBoundPS, UdpTransmitSocket &trnsmtSock)
{
	if (true == foundSensor)
	{
		getFrame();
		Mat depthMatOriginal(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);		
		Mat _2ndInteractnArea = depthMatOriginal.clone();
		config->applyConfigurationSettingsToMatrix(_2ndInteractnArea, 1);
		for (int j = 0; j < depthMatOriginal.rows; j++)
		{
			for (int i = 0; i < depthMatOriginal.cols; i++)
			{
				if (depthMatOriginal.at<uint16>(j, i) > 1300)
				{
					depthMatOriginal.at<uint16>(j, i) = emptyBoxMinReferrence;

				}
				else
				{
					if (depthMatOriginal.at<uint16>(j, i) > 1000 && depthMatOriginal.at<uint16>(j, i) != emptyBoxMinReferrence && !updatedSurface.empty())
					{
						updatedSurface.at<uint16>(j, i) = depthMatOriginal.at<uint16>(j, i);
					}
				}

			}
		}
		//printf("Pixel val: %d \n", mat.at<uint16>(200,200));
		Mat matCropped(config->cropRect[0].height, config->cropRect[0].width, CV_16U);
		Mat Mat2Cropped = updatedSurface.clone();	

		double vmin, vmax;
		int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };

		double _2nd_vmin, _2nd_vmax;
		int _2nd_idx_min[2] = { 255,255 }, _2nd_idx_max[2] = { 255, 255 };

		

		config->applyConfigurationSettingsToMatrix(depthMatOriginal, 0);
		

		minMaxIdx(depthMatOriginal, &vmin, &vmax, idx_min, idx_max);
		/*printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
			, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);*/

		minMaxIdx(_2ndInteractnArea, &_2nd_vmin, &_2nd_vmax, _2nd_idx_min, _2nd_idx_max);
		/*printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
		, _2nd_vmin, _2nd_vmax, _2nd_idx_min[1], _2nd_idx_min[0], _2nd_idx_max[1], _2nd_idx_max[0]);*/
		printf("MaxVal: %5.0f;  MaxX: %d MaxY %d\n", _2nd_vmax, _2nd_idx_max[1], _2nd_idx_max[0]);

		Mat BeforeColouredMat;
		Mat BeforeInvertedMat;
		Mat PlottedMat;
		int colmap = 4;
		cv::Mat DisplayMat;
		depthMatOriginal.convertTo(BeforeColouredMat, CV_8UC3);
		

	
		cv::applyColorMap(BeforeColouredMat, BeforeInvertedMat, colmap);
		cv::bitwise_not(BeforeInvertedMat, DisplayMat);
		
		

		// Second time arround with updating sand surface
		

		Mat BeforeColouredMat2;
		Mat BeforeInvertedMat2;
		Mat PlottedMat2;
		
		config->applyConfigurationSettingsToMatrix(Mat2Cropped, 0);

		Mat DisplayMat2;
		
		Mat2Cropped.convertTo(BeforeColouredMat2, CV_8UC3);
		
		
		applyColorMap(BeforeColouredMat2, DisplayMat2, colmap);
		//cv::bitwise_not(BeforeInvertedMat2, DisplayMat2);
			
		// scale the values to map to the parameter they control
		uint16 pitchVal = 48 + floor(double(35) / matCropped.cols * idx_max[1]);
		uint16 lpfVal = 1000 + floor(double(2000) / matCropped.cols * idx_max[0]); 

		// Send OSC when an event is triggered
		if (abs(vmax - currentMax) > 50 && vmax < 500) 
		{
			try 
			{
				printf("Triggering OSC send. Pitch: %d Cuttoff %d ", pitchVal, lpfVal);
				printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
					, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);
				outBoundPS	<< osc::BeginMessage("/t") << pitchVal << lpfVal << osc::EndMessage;
				trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
				outBoundPS.Clear();
			}
			catch(Exception e)
			{
				printf("Error: %s %s", e.err, e.msg);
			}
		}
		
		currentMax = vmax;		  
		flipAndDisplay(DisplayMat, "CvOutput", 2);
		flipAndDisplay(DisplayMat2, "CvOutput2", 2);

		SafeRelease(depthFrame);
	}
}

void App::Shutdown()
{
	// put cleaning up stuff here.
	delete[] depthBufferCurrentDepthFrame;
	delete[] depthBufferUpdatedSurface;
	SafeRelease(m_sensor);
	SafeRelease(m_depthFrameReader);
	cvDestroyWindow("CvOutput");
	free(depthBufferCurrentDepthFrame);
	free(depthBufferUpdatedSurface);
}