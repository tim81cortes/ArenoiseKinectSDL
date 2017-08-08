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
		namedWindow("DiffMapOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("DiffMapOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		
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
	
// ProcessInput
	// Initialise variables
		// Side control
	double _2nd_vmin, _2nd_vmax;
	int _2nd_idx_min[2] = { 0,0 }, _2nd_idx_max[2] = { 0, 0 };
	Mat _2ndInteractnArea;
	double scaledX, scaledY, scaledZ;
	double rectY;
	Scalar sideControlColor; Rect sideControlRect;

		// Channel 1 main
	double vmin, vmax;
	int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };
	Mat BeforeColouredMat;
	Mat BeforeInvertedMat;
	Mat DisplayMat;
	Mat beforeDisplayMat;
	int colmap = 4; // See openCV's colormap enumeration.
	int count = 0;

		// Channel 2 depth filter
	Mat Mat2Cropped;
	Mat BeforeColouredMat2;
	Mat BeforeInvertedMat2;
	Mat DisplayMat2;
	uint16 _2ndIntAreaWidth;

	// Aquire depthframe	
	getFrame();		
	Mat depthMatOriginal(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);		
	// Reduce size for processing
				// Channel 1 main
	medianBlur(depthMatOriginal.clone(), _2ndInteractnArea, 5);
	
	// Side control
	config->applyConfigurationSettingsToMatrix(_2ndInteractnArea, 1);

	// Filter noise 
		// Side control
	
		// main channel
	for (int j = 0; j < depthMatOriginal.rows; j++)
	{
		for (int i = 0; i < depthMatOriginal.cols; i++)
		{
			if (depthMatOriginal.at<uint16>(j, i) > 1300)
			{
				//printf("I: %d J: %d Val: %d\n", i, j, emptyBoxMinReferrence - depthMatOriginal.at<uint16>(j, i));
				depthMatOriginal.at<uint16>(j, i) = emptyBoxMinReferrence;

			}
			else
			{
				if (depthMatOriginal.at<uint16>(j, i) > 1000 && depthMatOriginal.at<uint16>(j, i) != emptyBoxMinReferrence)
				{
					updatedSurface.at<uint16>(j, i) = depthMatOriginal.at<uint16>(j, i);
				}
			}
		}
	}
	config->applyConfigurationSettingsToMatrix(depthMatOriginal, 0);
	
	Mat2Cropped = updatedSurface.clone();
	config->applyConfigurationSettingsToMatrix(Mat2Cropped, 0);
	// Calculate agregates
		// Side control area
	minMaxIdx(_2ndInteractnArea, &_2nd_vmin, &_2nd_vmax, _2nd_idx_min, _2nd_idx_max);
	scaledX =  _2nd_idx_max[1] * 256 / config->cropRect[1].width;
	scaledY = _2nd_idx_max[0] * 256 / config->cropRect[1].height;  
	scaledZ = _2nd_vmax * 256 / _2ndInteractnAreaMinReferrence;
	rectY = currentSideControlCoords[1] * config->cropRect[0].height / 256;
		// Channel 1 main
	minMaxIdx(depthMatOriginal, &vmin, &vmax, idx_min, idx_max);
	//printf("vmin: %3.0f, vmax: %3.0f\n", vmin, vmax );
// HandleEvents
	// Side control area
	if (scaledZ > 35 && scaledZ < 100)
	{	
		_3dCoordinates scaledCoords;
		scaledCoords.values[0] = scaledX;
		scaledCoords.values[1] = scaledY;
		scaledCoords.values[2] = scaledZ;
		
		
		sideControlColor = Scalar(0, scaledX, scaledZ);
		sideControlRect = Rect(0, rectY, config->cropRect[1].width, config->cropRect[0].height - rectY + 1);
		currentSideControlCoords[0] = scaledX;
		currentSideControlCoords[1] = scaledY;
		currentSideControlCoords[2] = scaledZ;
		DepthEvent handOverSideControl("HandOverSideControl", dpthEvent::evnt_Toggle, scaledCoords, 1);
		dpthEvntQ.emplace(handOverSideControl);
	}
	else
	{
		//DepthEvent endToggle("HandOverSideControl", dpthEvent::evnt_End);
		sideControlColor = Scalar(0, currentSideControlCoords[0], currentSideControlCoords[2]);
		sideControlRect = Rect(0, rectY, config->cropRect[1].width, config->cropRect[0].height - rectY + 1);
	}
		// Channel 1 main
	// Send OSC when an event is triggered
	uint16 pitchVal = 48 + floor(double(35) / depthMatOriginal.cols * idx_max[1]);
	uint16 lpfVal = 1000 + floor(double(2000) / depthMatOriginal.cols * idx_max[0]);

	

	if ((currentMax - vmax) > 100 && vmax < 500)
	{
		_3dCoordinates scaledCoords;
		scaledCoords.values[0] = idx_max[1] * 256 / config->cropRect[0].width;
		scaledCoords.values[1] = idx_max[0] * 256 / config->cropRect[0].height;
		scaledCoords.values[2] = vmax * 256 / emptyBoxMinReferrence;
		
		DepthEvent handOverSideControl("RemovedHandsFromSandBoxArea", dpthEvent::evnt_Toggle, scaledCoords, 1);
		dpthEvntQ.emplace(handOverSideControl);

		
	}
// Transmit OSC see above currently
// TODO Create packing loop
	while (!dpthEvntQ.empty())
	{
		DepthEvent tmp = dpthEvntQ.front();
		dpthEvntQ.pop();
		if (tmp.getEventName().compare("HandOverSideControl") == 0 || 0 == tmp.getEventName().compare("RemovedHandsFromSandBoxArea"))
		{
			_3dCoordinates _3dtmp2 = tmp.getCoordinates();
			printf("Event triggered Name: %s Type: %d X: %3.0f Y: %3.0f Z: %3.0f \n", tmp.getEventName().c_str(), tmp.getEventType(), _3dtmp2.values[0], _3dtmp2.values[1], _3dtmp2.values[2]);
		}
		if (0 == tmp.getEventName().compare("RemovedHandsFromSandBoxArea")) 
		{
			try
			{
				Mat differenceMap(depthMatOriginal.size(), CV_16U);
				DepthEvent dEvnt("DifferenceMapCreated", dpthEvent::evnt_persistent);
				if (!previousSurface.empty()) 
				{
					printf("previous surface now created.");
					subtract(previousSurface.clone(), depthMatOriginal, differenceMap);
				}

				previousSurface = depthMatOriginal.clone();
				//printf("Triggering OSC send. Pitch: %d Cuttoff %d ", pitchVal, lpfVal);
				//printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
				//, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);
				outBoundPS << osc::BeginMessage("/t") << pitchVal << lpfVal << osc::EndMessage;
				trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
				outBoundPS.Clear();
				Mat tmp;
				//bitwise_not(differenceMap,tmp);
				config->saveImage(differenceMap, count);
				for (int j = 0; j < differenceMap.rows; j++)
				{
					for (int i = 0; i < differenceMap.cols; i++)
					{
						if (differenceMap.at<uint16>(j, i) > 255)
						{
							//printf("I: %d J: %d Val: %d\n", i, j, emptyBoxMinReferrence - depthMatOriginal.at<uint16>(j, i));
							differenceMap.at<uint16>(j, i) = 0;
						}
						else if (differenceMap.at<uint16>(j, i) < 10)
						{
							differenceMap.at<uint16>(j, i) = 0;
						}
						else 
						{
							differenceMap.at<uint16>(j, i) = 65532;
						}
					}
				}
				differenceMap.convertTo(currentDifferenceMap, CV_8UC3);
				//threshold(differenceMap, currentDifferenceMap, cv::THRESH_TOZERO, 80, CV_8UC3);
				
				
				count++;


			}
			catch (Exception e)
			{
				printf("Error: %s %s", e.err, e.msg);
			}
			
		}
	}

// Update Knowledge
	currentMax = vmax;
	

// Render Screen

		// Channel 1 and side control
		
	
		depthMatOriginal.convertTo(BeforeColouredMat, CV_8UC3);
		addWeighted(BeforeColouredMat, 0.5 ,currentDifferenceMap, 0.5, 1.0, BeforeColouredMat);
		applyColorMap(BeforeColouredMat, BeforeInvertedMat, colmap);
		bitwise_not(BeforeInvertedMat, DisplayMat);		
		_2ndIntAreaWidth = config->cropRect[1].width;

		beforeDisplayMat = Mat(DisplayMat.rows, DisplayMat.cols + _2ndIntAreaWidth, CV_8UC3);

		DisplayMat.copyTo(beforeDisplayMat(Rect(config->cropRect[1].width, 0, DisplayMat.cols, DisplayMat.rows)));
		rectangle(beforeDisplayMat, sideControlRect, sideControlColor, CV_FILLED);
		
		flipAndDisplay(beforeDisplayMat, "CvOutput", 1);
		// Channel 2 
		Mat2Cropped.convertTo(BeforeColouredMat2, CV_8UC3);

		applyColorMap(BeforeColouredMat2, DisplayMat2, colmap);

		flipAndDisplay(DisplayMat2, "CvOutput2", 2);
		flipAndDisplay(currentDifferenceMap, "DiffMapOutput", 1);

		// Reporting
		SafeRelease(depthFrame);

		

		

		

		

		

		

		

		
		



		

		

 

		
		
		


	
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