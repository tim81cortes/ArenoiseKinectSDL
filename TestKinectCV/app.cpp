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
		namedWindow("CvOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		namedWindow("CvOutput2", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput2", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	}
}
void App::flipAndDisplay(Mat& toFlip, const String window, int wait)
{
	Mat toShow;
	flip(toFlip, toShow, 1);
	imshow(window, toShow);
	waitKey(wait);
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
		Mat _2ndInteractnArea;
		medianBlur(depthMatOriginal.clone(), _2ndInteractnArea, 5);
		config->applyConfigurationSettingsToMatrix(_2ndInteractnArea, 1);

		double _2nd_vmin, _2nd_vmax;
		int _2nd_idx_min[2] = { 0,0 }, _2nd_idx_max[2] = { 0, 0 };
		minMaxIdx(_2ndInteractnArea, &_2nd_vmin, &_2nd_vmax, _2nd_idx_min, _2nd_idx_max);
		if (abs(currentSideControlCoords[2] - _2nd_vmax) > 5 )		
		{
			currentSideControlCoords[2] = _2nd_vmax;
		}
		// Create a rectangle as tall as the sandbox config and as wide as the side control config
		double scaledX, scaledY, scaledZ;
		scaledX =  _2nd_idx_max[1] * 256 / config->cropRect[1].width;
		scaledY = _2nd_idx_max[0] * 256 / config->cropRect[1].height;  
		scaledZ = _2nd_vmax * 256 / _2ndInteractnAreaMinReferrence;
		

		double rectY = currentSideControlCoords[1] *  config->cropRect[0].height / 256;

		Scalar sideControlColor; Rect sideControlRect;
		if (scaledZ > 35 && scaledZ < 255)
		{
			sideControlColor = Scalar(0, scaledX, scaledZ);
			sideControlRect = Rect(0, rectY, config->cropRect[1].width, config->cropRect[0].height - rectY + 1);
			currentSideControlCoords[0] = scaledX;
			currentSideControlCoords[1] = scaledY;
			currentSideControlCoords[2] = scaledZ;
		}
		else
		{
			sideControlColor = Scalar(0, currentSideControlCoords[0], currentSideControlCoords[2]);
			sideControlRect = Rect(0, rectY, config->cropRect[1].width, config->cropRect[0].height - rectY + 1);
		}
			
		

		
		
		//Rect sideControlRect(0, _2nd_idx_max[0], config->cropRect[1].width, config->cropRect[0].height - _2nd_idx_max[0]);
		/*printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
		, _2nd_vmin, _2nd_vmax, _2nd_idx_min[1], _2nd_idx_min[0], _2nd_idx_max[1], _2nd_idx_max[0]);*/
		printf("MaxVal: %5.0f;  MaxX: %3.0f MaxY %3.0f \t", scaledZ, scaledX, scaledY);
		printf("MaxVal: %5.0f;  MaxX: %d MaxY %d\n", _2nd_vmax,_2nd_idx_max[1], _2nd_idx_max[0]);
		//printf("Rectx: %5.0f;  %3.0f\n", rectY);


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

		config->applyConfigurationSettingsToMatrix(depthMatOriginal, 0);
		
		minMaxIdx(depthMatOriginal, &vmin, &vmax, idx_min, idx_max);
		/*printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
			, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);*/

		Mat BeforeColouredMat;
		Mat BeforeInvertedMat;
		Mat PlottedMat;
		int colmap = 4;
		cv::Mat DisplayMat;
		depthMatOriginal.convertTo(BeforeColouredMat, CV_8UC3);
		
		applyColorMap(BeforeColouredMat, BeforeInvertedMat, colmap);
		bitwise_not(BeforeInvertedMat, DisplayMat);
		
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
		uint16 _2ndIntAreaWidth = config->cropRect[1].width;
		Mat beforeDisplayMat(DisplayMat.rows, DisplayMat.cols + _2ndIntAreaWidth,  CV_8UC3);
		
		DisplayMat.copyTo(beforeDisplayMat(Rect(config->cropRect[1].width,0,DisplayMat.cols,DisplayMat.rows)));
		rectangle(beforeDisplayMat, sideControlRect,sideControlColor,CV_FILLED);
		flipAndDisplay(beforeDisplayMat, "CvOutput", 2);
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