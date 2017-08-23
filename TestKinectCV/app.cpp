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
		
		double vmin, vmax;
		int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };
		
		

		cv::normalize(mat2, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
		NormMat.convertTo(DisplayMat, CV_8UC3);
		config->defineRegions(DisplayMat);	
		
		_2ndInteractnAreaMinReferrence = config->getZeroReferenceFromMatrix(mat2);
		
		config->applyConfigurationSettingsToMatrix(mat2, 0);
		mat2.convertTo(updatedSurface,CV_8UC3);
		mat2 = mat2(config->cropRect[2]);
		minMaxIdx(mat2, &vmin, &vmax, idx_min, idx_max);
		printf("Initial max %3.0f\n", vmax);
		initialMax = vmax;
		currentDifferenceMap = Mat(config->cropRect[0].size(), CV_8U);
		
		pMOG2 = cv::createBackgroundSubtractorMOG2();
		// Set flag to say that subsequent frames should be sent to different array
		initFrameDone = true;
		
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
	uint32 cumulativeDifferenceFromMin = 0;
	
	// Channel 2 depth filter
	Mat Mat2Cropped;
	Mat NormMat2;
	Mat BeforeColouredMat2;
	Mat BeforeInvertedMat2;
	Mat DisplayMat2;
	uint16 _2ndIntAreaWidth;
	_3dCoordinates scaledCoords;
	bool handsRaised = false;



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
	//for (int j = 0; j < depthMatOriginal.rows; j++)
	//{
	//	for (int i = 0; i < depthMatOriginal.cols; i++)
	//	{
	//		if (depthMatOriginal.at<uint16>(j, i) > 1300)
	//		{
	//			//printf("I: %d J: %d Val: %d\n", i, j, emptyBoxMinReferrence - depthMatOriginal.at<uint16>(j, i));
	//			//depthMatOriginal.at<uint16>(j, i) = emptyBoxMinReferrence;
	//		}
	//		else
	//		{
	//			if (depthMatOriginal.at<uint16>(j, i) > 1000 && depthMatOriginal.at<uint16>(j, i) != emptyBoxMinReferrence)
	//			{
	//				updatedSurface.at<uint16>(j, i) = depthMatOriginal.at<uint16>(j, i);
	//			}
	//		}
	//	}
	//}
	
	Mat2Cropped = depthMatOriginal.clone();

	config->applyConfigurationSettingsToMatrix(depthMatOriginal, 0);
	
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
	//printf("Max at front of pit: %3.0f \n", vmax );
	
		// Channel 2 
	Mat2Cropped.convertTo(BeforeColouredMat2, CV_8UC3, 0.25);
	
	int numBins = 128;

	bool uniform = true; bool accumulate = false;
	Mat depthHist;
	float range[] = { float(initialMax) * 0.25, 184 };
	const float* histRange = { range };
	calcHist(&BeforeColouredMat2, 1, 0, Mat(), depthHist, 1, &numBins, &histRange, uniform, accumulate);

	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound((double)hist_w / numBins);

	Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

	/// Normalize the result to [ 0, histImage.rows ]
	normalize(depthHist, depthHist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

	double hist_vmin, hist_vmax;
	int hist_idx_min[2] = { 255,255 }, hist_idx_max[2] = { 255, 255 };

	minMaxIdx(depthHist, &hist_vmin, &hist_vmax, hist_idx_min, hist_idx_max);
		

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
	
	// If there are no hands above the sandbox, restart the background subtracktion history
	if (currentMax  >  initialMax  &&  vmax  <  initialMax )
	{
		
		pMOG2 = cv::createBackgroundSubtractorMOG2();

	}
	
	// if first bin < maxval /4 trigger hands above sandpit event and store in knowledge
	if (depthHist.at<float>(0) * 4 < hist_vmax)
	{
		if (!handsCurrentlyRaisedAboveSand) 
		{
			//TODO define in terms of hands raised above sandbox
			_3dCoordinates scaledCoords;
			scaledCoords.values[0] = idx_max[1] * 256 / config->cropRect[0].width;
			scaledCoords.values[1] = idx_max[0] * 256 / config->cropRect[0].height;
			scaledCoords.values[2] = vmax * 256 / emptyBoxMinReferrence;

			DepthEvent handsRaisedAboveSand("HandsRaisedClearOfSand", dpthEvent::evnt_Toggle, scaledCoords, 1);
			objectOrientations.clear();
			dpthEvntQ.emplace(handsRaisedAboveSand);
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
			for (int i = 0; i < differenceMap.rows; i++)
			{
				for (int j = 0; j < differenceMap.cols; j++)
				{

					if (differenceMap.at<uint16>(i, j) > 255)
					{
						//printf("I: %d J: %d Val: %d\n", i, j, emptyBoxMinReferrence - depthMatOriginal.at<uint16>(j, i));
						differenceMap.at<uint16>(i, j) = 0;
					}
					else if (differenceMap.at<uint16>(i, j) < 10)
					{
						differenceMap.at<uint16>(i, j) = 0;
					}
					else
					{
						differenceMap.at<uint16>(i, j) = 65532;
					}
				}
			}

			differenceMap.convertTo(currentDifferenceMap, CV_8U);
			cvtColor(currentDifferenceMap.clone(), multi, COLOR_GRAY2BGR);
			cvtColor(multi, grey, COLOR_BGR2GRAY);
			threshold(grey, bw, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
			findContours(bw, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

			
		}
		handsRaised = true;
	}



	// Transmit OSC see above currently
// TODO Create packing loop
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
			try
			{
				//printf("Triggering OSC send. Pitch: %d Cuttoff %d ", pitchVal, lpfVal);
				//printf("MinVal: %5.0f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
				//, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);
				//outBoundPS << osc::BeginMessage("/t") << pitchVal << lpfVal << osc::EndMessage;
				//trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
				//outBoundPS.Clear();
			}
			catch (Exception e)
			{
				printf("Error: %s %s", e.err, e.msg);
			}
			printf("Event triggered Name: %s Type: %d X: %3.0f Y: %3.0f Z: %3.0f \n", tmp.getEventName().c_str(), tmp.getEventType(), _3dtmp2.values[0], _3dtmp2.values[1], _3dtmp2.values[2]);
		}
		if (0 == tmp.getEventName().compare("HandsRaisedClearOfSand"))
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
				//printf("Triggering OSC send. Pitch: %d Cuttoff %d ", pitchVal, lpfVal);
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
	handsCurrentlyRaisedAboveSand = handsRaised;

// Render Screen
	// Channel 2 
	createMask(depthMatOriginal);
		
	//printf("First bin: %3.0f, Max bin: %3.0f  \n", depthHist.at<float>(0), hist_vmax);

		
	cv::threshold(fgMaskMOG2, fgMaskMOG2, 254.0, 255.0,0);
	int dilation_size = 24; 
		
	subtract(fgMaskMOG2, currentDifferenceMap, fgMaskMOG2);
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE,
		cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		cv::Point(dilation_size, dilation_size));
	cv::dilate(fgMaskMOG2, fgMaskMOG2, element);
		
	Mat BeforeColouredMat3;
	depthMatOriginal.convertTo(BeforeColouredMat, CV_8UC3);
	Mat2Cropped.convertTo(BeforeColouredMat3, CV_8UC3);
	if (handsRaised)
	{
		for (int i = 0; i < BeforeColouredMat2.rows; i++)
			{
				for (int j = 0; j < BeforeColouredMat2.cols; j++ )
				{
					if (255 != fgMaskMOG2.at<uint8>(i,j)) 
					{
						//printf("Updating non-masked surface at i: %d j: %d \n", i, j);
						updatedSurface.at<uint8>(i, j) = BeforeColouredMat.at<uint8>(i,j);
					}
				}
			}
	}
	Mat bw2;
	if (!previouslyUpdatedSurface.empty())
	{
		// create difference map
		Mat differenceMapMasked;

		if (handsRaised)
		{
			if (!previouslyUpdatedSurface.empty())
			{
				subtract(previouslyUpdatedSurface.clone(), updatedSurface, differenceMapMasked);
			}

			previouslyUpdatedSurface = updatedSurface.clone();
			threshold(differenceMapMasked, bw, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
			findContours(bw, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
			for (int i = 0; i < contours.size(); ++i)
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

	}
	cv::applyColorMap(updatedSurface, DisplayMat2, colmap);
	if (!bw2.empty())
	{
		flipAndDisplay(bw2, "CvOutput2", 1);
	}
	

		
	// Channel 1 and side control		
		
		
	cv::addWeighted(BeforeColouredMat, 0.5 ,currentDifferenceMap, 0.5, 1.0, BeforeColouredMat);
	cv::applyColorMap(BeforeColouredMat, DisplayMat, colmap);
		
	for (int i = 0; i < objectOrientations.size(); ++i)
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



	flipAndDisplay(currentDifferenceMap, "DiffMapOutput", 1);
		
	// Reporting
	SafeRelease(depthFrame);
}


void App::drawAxis(Mat& img, Point p, Point q, Scalar colour, const float scale)
{
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
	pMOG2->apply(src, fgMaskMOG2);
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