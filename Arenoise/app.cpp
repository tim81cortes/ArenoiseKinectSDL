 // File: app.cpp
 // Name: Tim Aylott.cpp
 // Date: 01/09/2017
 // Course: MSc Software Systems
 // Description: Arenoise is a Digital Musical Instrument which 
 // uses sand in a sandbox as an interaction meduim. This is 
 // component 1 which reads the sensor data, processes interaction events, 
 // sends them to component 2 and then displays visual feedback. 
// This file has a follows the game loop pattern to process depth data
// into musical events. It then renders a visualisa of the input and OSC output.

#include "App.h"	

void App::Init()
{	
	config = new Configure(Rect(30, 30, 30, 30), Point(0, 0), Point(0, 0));
	// Load a frame with the empty sandbox values
	String filename = "EmptySandboxInteractionArea1.xml";
	emptyBoxMinReferrence = config->getZeroReferenceFromFile(filename);
	
	HRESULT hr;
	BOOLEAN *kinectAvailability = false;

	// Check that there's a sensor and end the program if not.
	hr = GetDefaultKinectSensor(&m_sensor);
	m_sensor->get_IsAvailable(kinectAvailability);
	if (FAILED(hr))
	{
		printf("Failed to find correct sensor.\n");
		foundSensor = false;
		exit(2);
	}
	m_sensor->Open();
	// Get the depth frame reader
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
	// Once the reader is aquired there is no need to keep the source
	SafeRelease(depthFrameSource);
	
	// Set buffer for frame storage
	depthBufferCurrentDepthFrame = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];

	double vmin, vmax;

		int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };

	// Wait until a depthframe has been aquired.
	while(!getFrame());
	Mat mat2(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);
	Mat NormMat;
	Mat DisplayMat;
	
	cv::normalize(mat2, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
	mat2.copyTo(updatedSurface);
	NormMat.convertTo(DisplayMat, CV_8UC3);
	// Defing interaction area. Requests new config if none exists
	config->defineRegions(DisplayMat, mat2);
	// Retrieves data from config instance
	configuredSandboxRimHeight = config->getInitialHandsRemovedRoIMax();
	_2ndInteractnAreaMinReferrence = config->getZeroReferenceFromMatrix(mat2);

	// Display configuration data 
	printf("Initial max %d\n", configuredSandboxRimHeight);
	currentDifferenceMap = Mat(config->cropRect[0].size(), CV_8U);
		
	// Set flag to say that subsequent frames should be sent to different array
	initFrameDone = true;
		
	SafeRelease(depthFrame);
	namedWindow("CvOutput", CV_WINDOW_NORMAL);
	cvSetWindowProperty("CvOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	cvDestroyWindow("Choose interaction area then highlight dead pixels.");

}

void App::Tick(float deltaTime, osc::OutboundPacketStream &outBoundPS, UdpTransmitSocket &trnsmtSock)
{
	
// =============		ProcessInput		================
	// Initialise local variables
	// Channel 1 main
	double vmin, vmax;
	int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };
	Mat beforeColouredMatrix;
	Mat beforeInvertedMatrix;
	Mat displayMatrix;
	int colmap = 4; // See openCV's colormap enumeration.
	int count = 0;
	Mat bw;
	Mat multi;
	Mat grey;
	
	// Side control
	double _side_vmin, _side_vmax;
	int _side_idx_min[2] = { 0,0 }, _side_idx_max[2] = { 0, 0 };
	Mat _2ndInteractnArea;
	double scaledX, scaledY, scaledZ;
	double rectY;
	Scalar sideControlColor; Rect sideControlRect;
	_3dCoordinates scaledCoords;
	
	// Do nothing unless a depthframe is aquired	
	if (!getFrame())
	{
		return;
	}
	
	Mat depthMatOriginal(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);		
	// Channel 1 main
	medianBlur(depthMatOriginal.clone(), _2ndInteractnArea, 5);
	
	// Side control
	config->applyConfigurationSettingsToMatrix(_2ndInteractnArea, 1);
	
	// Applys zero reference and blurs individual anomolous pixels
	config->applyConfigurationSettingsToMatrix(depthMatOriginal, 0);
	
//	=============	Calculate agregates	===========
	//	Side control area
	minMaxIdx(_2ndInteractnArea, &_side_vmin, &_side_vmax, _side_idx_min, _side_idx_max);
	// Store aggregates in coordinate struct array
	scaledCoords.values[0] =  _side_idx_max[1] * 256 / config->cropRect[1].width;
	scaledCoords.values[1] = _side_idx_max[0] * 256 / config->cropRect[1].height;
	scaledCoords.values[2] = _side_vmax * 256 / _2ndInteractnAreaMinReferrence;
	
		// Channel 1 main
	Mat aggragateMat = Mat(depthMatOriginal.clone(), config->cropRect[2]);
	minMaxIdx(aggragateMat, &vmin, &vmax, idx_min, idx_max);
	//printf("Max at front of pit: %d CurrentMax: %d  Config rim height: %d \n", uint16(vmax), currentMax, configuredSandboxRimHeight);
// ===============		Discover events		===============
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
	if (currentMax > configuredSandboxRimHeight && vmax < configuredSandboxRimHeight )
	{
		// Create object for event queue
		_3dCoordinates scaledCoords;
		scaledCoords.values[0] = idx_max[1] * 256 / config->cropRect[0].width;
		scaledCoords.values[1] = idx_max[0] * 256 / config->cropRect[0].height;
		scaledCoords.values[2] = vmax * 256 / emptyBoxMinReferrence;
		
		DepthEvent removedHandsFromSandboxArea("RemovedHandsFromSandboxArea", dpthEvent::evnt_Toggle, scaledCoords, 1);
		objectOrientations.clear();
		dpthEvntQ.emplace(removedHandsFromSandboxArea);
		Mat differenceMap(depthMatOriginal.size(), CV_16U);
		if (!previousSurface.empty())
		{
			// create difference map
			subtract(previousSurface.clone(), depthMatOriginal, differenceMap);
		}
		// previousSurface is populated so that frame differenc can occur on next loop
		previousSurface = depthMatOriginal.clone();
		
		Mat tmp;
		// Using iterative method to threshold image helps remove the grey
		for (int j = 0; j < differenceMap.rows; j++)
		{
			for (int i = 0; i < differenceMap.cols; i++)
			{
				if (differenceMap.at<uint16>(j, i) > 255)
				{
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
		// Process differnce map as required by findContours function
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
			drawContours(displayMatrix, contours, static_cast<int>(i), Scalar(0, 255, 0), 2, 8, hierarchy, 0);
			
			// Find the orientation of each shape
			getOrientation(contours[i], displayMatrix, objOrient);

			// Calculate the distance between points
			objOrient.distFromCentre[0] = euclideanDist(Point(objOrient.center[0], objOrient.center[1]), Point(objOrient.front[0], objOrient.front[1]));
			objOrient.distFromCentre[1] = euclideanDist(Point(objOrient.center[0], objOrient.center[1]), Point(objOrient.side[0], objOrient.side[1]));
			objectOrientations.push_back(objOrient);

			DepthEvent foundDiffObj("DifferenceObjectFound", dpthEvent::evnt_Toggle, objOrient, 1);
			dpthEvntQ.emplace(foundDiffObj);
		}
	}

//	=============	Handle events in event queue	============
	while (!dpthEvntQ.empty())
	{
		DepthEvent tmp = dpthEvntQ.front();
		dpthEvntQ.pop();
		if (0 == tmp.getEventName().compare("HandOverSideControl") )
		{
			// Event sents control message for LFO over OSC packet stream
			_3dCoordinates _3dtmp2 = tmp.getCoordinates();
			uint16 rate = _3dtmp2.values[2] * 10 / 65;
			uint16 depth = 320 - _3dtmp2.values[1] * 320 / 255;
			outBoundPS << osc::BeginMessage("/lfo") << rate << depth << osc::EndMessage;
			trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
			outBoundPS.Clear();			
			printf("Event triggered Name: %s Type: %d X: %3.0f Y: %3.0f Z: %3.0f \n", tmp.getEventName().c_str(), tmp.getEventType(), _3dtmp2.values[0], _3dtmp2.values[1], _3dtmp2.values[2]);
		}
		if (0 == tmp.getEventName().compare("RemovedHandsFromSandboxArea")) 
		{
			// Event currently unused in its own right but useful to display when it occurs
			_3dCoordinates _3dtmp2 = tmp.getCoordinates();
			printf("Event triggered Name: %s Type: %d X: %3.0f Y: %3.0f Z: %3.0f \n", tmp.getEventName().c_str(), tmp.getEventType(), _3dtmp2.values[0], _3dtmp2.values[1], _3dtmp2.values[2]);
		}
		if (0 == tmp.getEventName().compare("DifferenceObjectFound"))
		{
			// Event sents control message for main note controller 
			// over OSC packet stream after scaling is applied
			orientationVector tmpOrVect = tmp.getOrientationVector();
			uint16 pitch = 45 + tmpOrVect.center[0] * 45 / config->cropRect[0].width;
			uint16 cutoffLPF = 500 + tmpOrVect.center[1] * 1500 / config->cropRect[0].height;
			uint16 duration = tmpOrVect.distFromCentre[0] * 500 / 100;
			uint16 amplitude = tmpOrVect.distFromCentre[1]; //* 100 / 100
			printf("Event triggered Name: %s Type: %d X: %d Y: %d\n", tmp.getEventName().c_str(), tmp.getEventType(), tmpOrVect.front[0], tmpOrVect.front[1]);

			try
			{
				// Send OSC over packet stream
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

// ==============	Update remaining knowledge	============
		currentMax = vmax;

//	=============			Render Screen		==================

// HISTOGRAM_VISUALISATION_SNIPPET_1
		Mat MatCropped = depthMatOriginal.clone();
		Mat BeforeColouredMat;
		MatCropped.convertTo(BeforeColouredMat, CV_8UC3, 0.25);
		int numBins = 128;

		bool uniform = true; bool accumulate = false;
		Mat depthHist;
		float range[] = { float(configuredSandboxRimHeight) * 0.25, 184 };
		const float* histRange = { range };
		calcHist(&BeforeColouredMat, 1, 0, Mat(), depthHist, 1, &numBins, &histRange, uniform, accumulate);

		int hist_w = 512; int hist_h = 400;
		int bin_w = cvRound((double)hist_w / numBins);

		Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

		// Normalize the result to clearly display the graph
		normalize(depthHist, depthHist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

		double hist_vmin, hist_vmax;
		int hist_idx_min[2] = { 255,255 }, hist_idx_max[2] = { 255, 255 };

		minMaxIdx(depthHist, &hist_vmin, &hist_vmax, hist_idx_min, hist_idx_max);

		// Draw the lines on the histImage matrix
		for (int i = 1; i < numBins; i++)
		{
			line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(depthHist.at<float>(i - 1))),
				Point(bin_w*(i), hist_h - cvRound(depthHist.at<float>(i))),
				Scalar(255, 0, 0), 2, 8, 0);
		}

		line(histImage, Point(configuredSandboxRimHeight * 0.25, 0), Point(configuredSandboxRimHeight * 0.25, hist_h - 100), Scalar(0, 255, 255));
		line(histImage, Point(hist_idx_max[0] * 4, 0), Point(hist_idx_max[0] * 4, hist_h - 100), Scalar(0, 255, 0));


		// Channel 1 and side control		
		// Display the difference map on the user interface and add colour depth mapping
		depthMatOriginal.convertTo(beforeColouredMatrix, CV_8UC3);
		addWeighted(beforeColouredMatrix, 0.5 ,currentDifferenceMap, 0.5, 1.0, beforeColouredMatrix);
		applyColorMap(beforeColouredMatrix, displayMatrix, colmap);
		// Draw points and axes on to the display
		for (size_t i = 0; i < objectOrientations.size(); ++i)
		{
			Point center(objectOrientations[i].center[0], objectOrientations[i].center[1]);
			Point front(objectOrientations[i].front[0], objectOrientations[i].front[1]);
			Point side(objectOrientations[i].side[0], objectOrientations[i].side[1]);
			drawAxis(displayMatrix, center, front, Scalar(0, 0, 255), 1);
			drawAxis(displayMatrix, center, side, Scalar(255, 255, 0), 5);
			circle(displayMatrix, center, 3, Scalar(255, 0, 255), 2);
		}

		rectangle(displayMatrix, sideControlRect, sideControlColor, CV_FILLED);
		flipAndDisplay(displayMatrix, "CvOutput", 1);
		//flipAndDisplay(histImage, "CvOutput", 1);
		SafeRelease(depthFrame);
}

void App::drawAxis(Mat& img, Point p, Point q, Scalar colour, const float scale)
{
	// Code from tutorial in OpenCV originally by Svetlin Penkov 
	double angle;
	double hypotenuse;
	angle = atan2((double)p.y - q.y, (double)p.x - q.x); // angle in radians
	hypotenuse = sqrt((double)(p.y - q.y) * (p.y - q.y) + (p.x - q.x) * (p.x - q.x));
	//    double degrees = angle * 180 / CV_PI; // convert radians to degrees (0-180 range)
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
	// Code from tutorial in OpenCV originally by Svetlin Penkov 
	// See Masters thesis for referrence

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
	delete config;
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