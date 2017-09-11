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
		
		
		cv::normalize(mat2, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
		_2ndInteractnAreaMinReferrence = config->getZeroReferenceFromMatrix(mat2);
		mat2.copyTo(updatedSurface);
		NormMat.convertTo(DisplayMat, CV_8UC3);
		config->defineRegions(DisplayMat, mat2);	
		configuredSandboxRimHeight = config->getInitialHandsRemovedRoIMax();
		
		printf("Initial max %d\n", configuredSandboxRimHeight);
		currentDifferenceMap = Mat(config->cropRect[0].size(), CV_8U);
		

		// Set flag to say that subsequent frames should be sent to different array
			
		SafeRelease(depthFrame);
		namedWindow("CvOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		namedWindow("CvOutput2", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput2", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		namedWindow("DiffMapOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("DiffMapOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		cvDestroyWindow("Choose interaction area then highlight dead pixels.");
		
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
	return foundSensor;
}
void App::Tick(float deltaTime, osc::OutboundPacketStream &outBoundPS, UdpTransmitSocket &trnsmtSock)
{
	
// ProcessInput
	// Initialise local variables
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
	Mat bw;
	Mat multi;
	Mat grey;
	
	// Channel 2 depth filter
	Mat Mat2Cropped;
	Mat BeforeColouredMat2;
	Mat BeforeInvertedMat2;
	Mat DisplayMat2;
	uint16 _2ndIntAreaWidth;
	_3dCoordinates scaledCoords;



	// Aquire depthframe and save into local buffer	
	getFrame();		
	// Create matrix and copy depth frame from buffer
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
			if (depthMatOriginal.at<uint16>(j, i) < 1300)
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
	scaledCoords.values[0] =  _2nd_idx_max[1] * 256 / config->cropRect[1].width;
	scaledCoords.values[1] = _2nd_idx_max[0] * 256 / config->cropRect[1].height;
	scaledCoords.values[2] = _2nd_vmax * 256 / _2ndInteractnAreaMinReferrence;
	
		// Channel 1 main
	Mat aggragateMat = Mat(depthMatOriginal.clone(), config->cropRect[2]);
	minMaxIdx(aggragateMat, &vmin, &vmax, idx_min, idx_max);
	//printf("Max at front of pit: %d \n", uint16(vmax));
// HandleEvents
	// Side control area
	if (scaledCoords.values[2] > 35 && scaledCoords.values[2] < 100)
	{	
		rectY = scaledCoords.values[1] * config->cropRect[0].height / 256 - config->cropRect[1].width;
		sideControlColor = Scalar(0, scaledCoords.values[0], scaledCoords.values[2]);
		sideControlRect = Rect(0, rectY, config->cropRect[1].width, config->cropRect[0].height - rectY + 1);
		currentSideControlCoords[0] = scaledCoords.values[0];
		currentSideControlCoords[1] = scaledCoords.values[1];
		currentSideControlCoords[2] = scaledCoords.values[2];
		DepthEvent handOverSideControl("HandOverSideControl", dpthEvent::evnt_Toggle, scaledCoords, 1);
		dpthEvntQ.emplace(handOverSideControl);
	}
	else
	{	
		rectY = currentSideControlCoords[1] * config->cropRect[0].height / 256;
		sideControlColor = Scalar(0, currentSideControlCoords[0], currentSideControlCoords[2]);
		sideControlRect = Rect(0, rectY, config->cropRect[1].width, config->cropRect[0].height - rectY + 1);
	}
		// Channel 1 main
	// Send OSC when an event is triggered
	uint16 pitchVal = 48 + floor(double(35) / depthMatOriginal.cols * idx_max[1]);
	uint16 lpfVal = 1000 + floor(double(2000) / depthMatOriginal.cols * idx_max[0]);

	if (uint16(vmax) < configuredSandboxRimHeight && currentMax > configuredSandboxRimHeight)
	{
		_3dCoordinates scaledCoords;
		scaledCoords.values[0] = idx_max[1] * 256 / config->cropRect[0].width;
		scaledCoords.values[1] = idx_max[0] * 256 / config->cropRect[0].height;
		scaledCoords.values[2] = vmax * 256 / emptyBoxMinReferrence;
		
		DepthEvent handOverSideControl("RemovedHandsFromSandBoxArea", dpthEvent::evnt_Toggle, scaledCoords, 1);
		objectOrientations.clear();
		dpthEvntQ.emplace(handOverSideControl);
		Mat differenceMap(depthMatOriginal.size(), CV_16U);
		if (!previousSurface.empty())
		{
			// create difference map
			subtract(previousSurface.clone(), depthMatOriginal, differenceMap);
			//subtract(depthMatOriginal.clone(), previousSurface, differenceMap);
		}
		previousSurface = depthMatOriginal.clone();
		// Try simple thresholding
		// TODO add else condition to make sure that the differenceMap is only iterrated over after
		// previousSurface is populated.
		Mat tmp;
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

		differenceMap.convertTo(currentDifferenceMap, CV_8U);
		cvtColor(currentDifferenceMap.clone(), multi, COLOR_GRAY2BGR);
		cvtColor(multi, grey, COLOR_BGR2GRAY);
		threshold(grey, bw, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
		findContours(bw, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
		
		for (size_t i = 0; i < contours.size(); ++i)
		{
			orientationVector objOrient;
			// Calculate the area of each contour
			double area = contourArea(contours[i]);
			// Ignore contours that are too small or too large
			if (area < 1e2 || 1e5 < area)
			{
				continue;
			}
			// Draw each contour only for visualisation purposes
			drawContours(DisplayMat, contours, static_cast<int>(i), Scalar(0, 255, 0), 2, 8, hierarchy, 0);
			// Find the orientation of each shape

			getOrientation(contours[i], DisplayMat, objOrient);

			// Calculate the distance between points
			objOrient.distFromCentre[0] = euclideanDist(Point(objOrient.center[0], objOrient.center[1]), Point(objOrient.front[0], objOrient.front[1]));
			objOrient.distFromCentre[1] = euclideanDist(Point(objOrient.center[0], objOrient.center[1]), Point(objOrient.side[0], objOrient.side[1]));
			objectOrientations.push_back(objOrient);
			
			DepthEvent foundDiffObj("DifferenceObjectFound", dpthEvent::evnt_Toggle, objOrient, 1);
			dpthEvntQ.emplace(foundDiffObj);
		}


	}
// Transmit OSC see above currently

	while (!dpthEvntQ.empty())
	{
		DepthEvent tmp = dpthEvntQ.front();
		dpthEvntQ.pop();
		if (0 == tmp.getEventName().compare("HandOverSideControl") )
		{

			_3dCoordinates _3dtmp2 = tmp.getCoordinates();
			uint16 rate = _3dtmp2.values[2] * 10 / 65;
			uint16 depth = 320 - _3dtmp2.values[1] * 320 / 255;
			printf("Event triggered Name: %s Type: %d X: %3.0f Y: %3.0f Z: %3.0f \n", tmp.getEventName().c_str(), tmp.getEventType(), _3dtmp2.values[0], _3dtmp2.values[1], _3dtmp2.values[2]);
			outBoundPS << osc::BeginMessage("/lfo") << rate << depth << osc::EndMessage;
			trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
			outBoundPS.Clear();
		}
		if (0 == tmp.getEventName().compare("RemovedHandsFromSandBoxArea")) 
		{
			_3dCoordinates _3dtmp2 = tmp.getCoordinates();

			printf("Event triggered Name: %s Type: %d X: %3.0f Y: %3.0f Z: %3.0f \n", tmp.getEventName().c_str(), tmp.getEventType(), _3dtmp2.values[0], _3dtmp2.values[1], _3dtmp2.values[2]);
		}
		if (0 == tmp.getEventName().compare("DifferenceObjectFound"))
		{
			orientationVector tmpOrVect = tmp.getOrientationVector();
			printf("Event triggered Name: %s Type: %d X: %d Y: %d\n", tmp.getEventName().c_str(), tmp.getEventType(), tmpOrVect.front[0], tmpOrVect.front[1]);
			uint16 pitch = 45 + tmpOrVect.center[0] * 45 / config->cropRect[0].width;
			uint16 cutoffLPF = 500 + tmpOrVect.center[1] * 1500 / config->cropRect[0].height;
			uint16 duration = tmpOrVect.distFromCentre[0] * 500 / 100;
			uint16 amplitude = tmpOrVect.distFromCentre[1]; //* 100 / 100

			try
			{
				outBoundPS << osc::BeginMessage("/t") << pitch << cutoffLPF << duration << amplitude <<osc::EndMessage;
				trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
				outBoundPS.Clear();
				
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
		// Channel 2 
		Mat2Cropped.convertTo(BeforeColouredMat2, CV_8UC3);
		applyColorMap(BeforeColouredMat2, DisplayMat2, colmap);
		
		flipAndDisplay(DisplayMat2, "CvOutput2", 1);

		
		// Channel 1 and side control		
		
		
		depthMatOriginal.convertTo(BeforeColouredMat, CV_8UC3);

		addWeighted(BeforeColouredMat, 0.5 ,currentDifferenceMap, 0.5, 1.0, BeforeColouredMat);

		applyColorMap(BeforeColouredMat, DisplayMat, colmap);


		for (size_t i = 0; i < objectOrientations.size(); ++i)
		{
			
			Point center(objectOrientations[i].center[0], objectOrientations[i].center[1]);
			Point front(objectOrientations[i].front[0], objectOrientations[i].front[1]);
			Point side(objectOrientations[i].side[0], objectOrientations[i].side[1]);
			drawAxis(DisplayMat, center, front, Scalar(0, 0, 255), 1);
			drawAxis(DisplayMat, center, side, Scalar(255, 255, 0), 5);
			circle(DisplayMat, center, 3, Scalar(255, 0, 255), 2);
			
		}

		
		_2ndIntAreaWidth = config->cropRect[1].width;
		beforeDisplayMat = Mat(DisplayMat.rows, DisplayMat.cols, CV_8UC3);
		DisplayMat.copyTo(beforeDisplayMat(Rect(0, 0, DisplayMat.cols, DisplayMat.rows)));
		cv::rectangle(beforeDisplayMat, sideControlRect, sideControlColor, CV_FILLED);
		flipAndDisplay(beforeDisplayMat, "CvOutput", 1);


		if (!bw.empty()) 
		{
			flipAndDisplay(currentDifferenceMap, "DiffMapOutput", 1);
		}
	
		SafeRelease(depthFrame);
}


void App::drawAxis(Mat& img, Point p, Point q, Scalar colour, const float scale)
{
	double angle;
	double hypotenuse;
	angle = atan2((double)p.y - q.y, (double)p.x - q.x); // angle in radians
	hypotenuse = sqrt((double)(p.y - q.y) * (p.y - q.y) + (p.x - q.x) * (p.x - q.x));
	//    double degrees = angle * 180 / CV_PI; // use to convert radians to degrees (0-180 range)
	//    cout << "Degrees: " << abs(degrees - 180) << endl; // angle in 0-360 degrees range

	// Here we lengthen the arrow by a factor of scale
	q.x = (int)(p.x - scale * hypotenuse * cos(angle));
	q.y = (int)(p.y - scale * hypotenuse * sin(angle));
	line(img, p, q, colour, 1, CV_AA);

	// create the arrow hooks
	p.x = (int)(q.x + 9 * cos(angle + CV_PI / 4));
	p.y = (int)(q.y + 9 * sin(angle + CV_PI / 4));
	line(img, p, q, colour, 1, CV_AA);

	p.x = (int)(q.x + 9 * cos(angle - CV_PI / 4));
	p.y = (int)(q.y + 9 * sin(angle - CV_PI / 4));
	line(img, p, q, colour, 1, CV_AA);
}

double App::getOrientation(const std::vector<Point> &pts, Mat &img, orientationVector &orVect)
{
	//Construct a buffer used by the pca analysis
	int sz = static_cast<int>(pts.size());
	Mat data_pts = Mat(sz, 2, CV_64FC1);
	for (int i = 0; i < data_pts.rows; ++i)
	{
		data_pts.at<double>(i, 0) = pts[i].x;
		data_pts.at<double>(i, 1) = pts[i].y;
	}

	//Perform PCA analysis
	PCA pca_analysis(data_pts, Mat(), CV_PCA_DATA_AS_ROW);

	//Store the center of the object
	Point cntr = Point(static_cast<int>(pca_analysis.mean.at<double>(0, 0)),
		static_cast<int>(pca_analysis.mean.at<double>(0, 1)));

	//Store the eigenvalues and eigenvectors
	std::vector<Point2d> eigen_vecs(2);
	std::vector<double> eigen_val(2);
	for (int i = 0; i < 2; ++i)
	{
		eigen_vecs[i] = Point2d(pca_analysis.eigenvectors.at<double>(i, 0),
			pca_analysis.eigenvectors.at<double>(i, 1));

		eigen_val[i] = pca_analysis.eigenvalues.at<double>(0, i);
	}
	// Draw the principal components
	Point p1 = cntr + 0.02 * Point(static_cast<int>(eigen_vecs[0].x * eigen_val[0]), static_cast<int>(eigen_vecs[0].y * eigen_val[0]));
	Point p2 = cntr - 0.02 * Point(static_cast<int>(eigen_vecs[1].x * eigen_val[1]), static_cast<int>(eigen_vecs[1].y * eigen_val[1]));
	orVect.center[0] = cntr.x;
	orVect.center[1] = cntr.y;
	orVect.front[0] = p1.x;
	orVect.front[1] = p1.y;
	orVect.side[0] = p2.x;
	orVect.side[1] = p2.y;

	double angle = atan2(eigen_vecs[0].y, eigen_vecs[0].x); // orientation in radians

	return angle;

}

double App::euclideanDist(Point& p, Point& q) {
	Point diff = p - q;
	return cv::sqrt(diff.x*diff.x + diff.y*diff.y);
}

void App::createMask(Mat & src)
{
	//update the background model
	//pMOG->apply(src.clone(), fgMaskMOG);
	//pMOG2->apply(src, fgMaskMOG2);
	//get the frame number and write it on the current frame
	
	
	

}



void App::Shutdown()
{
	// put cleaning up stuff here.
	delete[] depthBufferCurrentDepthFrame;
	delete[] depthBufferUpdatedSurface;
	SafeRelease(m_sensor);
	SafeRelease(m_depthFrameReader);
	cvDestroyWindow("CvOutput");
	cvDestroyWindow("CvOutput2");
	cvDestroyWindow("DiffMapOutput");
	free(depthBufferCurrentDepthFrame);
	free(depthBufferUpdatedSurface);
}