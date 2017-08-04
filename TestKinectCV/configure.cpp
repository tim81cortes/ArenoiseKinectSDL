#include "configureApp.h"

Configure::Configure(Rect crpRct, Point pnt1, Point pnt2)
{
	cropRect[0] = crpRct;
	cropRect[1] = crpRct;
	P1 = pnt1;
	P2 = pnt2;
}

void Configure::onMouse(int event, int x, int y) {
// TODO Refactor to remove boolean variables where possible.
	switch (event) {

	case  CV_EVENT_LBUTTONDOWN:
		justReleased = false;
		wasClicked = true;
		P1.x = x;
		P1.y = y;
		printf("ButtonDown");
		break;

	case  CV_EVENT_LBUTTONUP:
		P2.x = x;
		P2.y = y;
		if (wasClicked)
		{
			// Only send one event once released
			justReleased = true;
			wasClicked = false;
		}
		else 
		{
			justReleased = false;
		}
		
		printf("ButtonUp");
		break;



	default:   break;


	}

	if (justReleased && CV_EVENT_LBUTTONUP == event) 
	{
		// 
		Rect tempRect;
		if (P1.x>P2.x) 
		{
			tempRect.x = P2.x;
			tempRect.width = P1.x - P2.x;
		}
		else 
		{
			tempRect.x = P1.x;
			tempRect.width = P2.x - P1.x;
		}

		if (P1.y>P2.y) 
		{
			tempRect.y = P2.y;
			tempRect.height = P1.y - P2.y;
		}
		else 
		{
			tempRect.y = P1.y;
			tempRect.height = P2.y - P1.y;
		}
		if (!displayAreaSet)
		{
			cropRect[0] = tempRect;
			printf("Interaction area 1 set as X: %d Y: %d W: %d H: %d \n", cropRect[0].x, cropRect[0].y, cropRect[0].width, cropRect[0].height);
			displayAreaSet = true;
		}
		else if(displayAreaSet && !sideOfBoxAreaSet)
		{
			cropRect[1] = tempRect;
			printf("Interaction area 2 set as X: %d Y: %d W: %d H: %d \n", cropRect[1].x, cropRect[1].y, cropRect[1].width, cropRect[1].height);
			sideOfBoxAreaSet = true;
		}
		else
		{
			rectangles.push_back(tempRect);
			printf("Rectangle added for blurring. X: %d Y: %d W: %d H: %d \n", tempRect.x, tempRect.y, tempRect.width, tempRect.height);
		}

	}

}

void Configure::onMouse(int event, int x, int y, int f, void* userData) {
	if (userData == nullptr)
	{
		printf("There was an error cause by lack of user data in staic onMouse member function.\n");
		return;
	}

	Configure* config = reinterpret_cast<Configure*>(userData);
	config->onMouse(event, x, y);
	}

void Configure::defineRegions(Mat& capturedImage) {
	
	namedWindow("Choose interaction area then highlight dead pixels.", CV_WINDOW_NORMAL);
	do {
		setMouseCallback("Choose interaction area then highlight dead pixels.", onMouse, this);
		imshow("Choose interaction area then highlight dead pixels.", capturedImage);
		
	} while (cvWaitKey(0) != 27);
	

}
	void Configure::applyConfigurationSettingsToMatrix(Mat& src, int whichArea) {
	Mat ROI; // Secondary interaction area i.e. the white platform next to the pit
 

	checkBoundary(src);
	
	for (int i = 0; i < rectangles.size(); i++)
	{
		if (rectangles[i].width>0 && rectangles[i].height>0) {
			medianBlur(src(rectangles[i]), src(rectangles[i]), 5);
		}
	}

		if (cropRect[whichArea].width>0 && cropRect[whichArea].height>0) {
			ROI = src(cropRect[whichArea]);
		}
		
		if (whichArea) {

			printf("Zero referencing area %d the matrix from value %d", whichArea , boxBottom[whichArea]);
		}

	subtract((boxBottom[whichArea]), ROI, ROI);
	src = ROI;
}



int Configure::getZeroReferenceFromFile(String depthFrameName) {
	FileStorage fs;
	Mat emptyBoxFromFile;
	String qualifiedDepthFrameName = "ConfigurationFiles\\" + depthFrameName;

	try
	{
		FileStorage file(qualifiedDepthFrameName, FileStorage::READ);
		file["EmptySandboxWholeDepthFrame"] >> emptyBoxFromFile;
	}
	catch (Exception e)
	{
		printf("There was an error loading the configuration file. %s \n", e.msg);
		exit(1);
	}
	
	fs.release();
	boxBottom[0] = getZeroReference(emptyBoxFromFile);
}

int Configure::getZeroReferenceFromMatrix(Mat src) {
	Mat ROI;

	if (cropRect[1].width>0 && cropRect[1].height>0) {
		ROI = src(cropRect[1]);
	}
	boxBottom[1] = getZeroReference(ROI);
	return boxBottom[1];
}



void Configure::checkBoundary(Mat& src) {
	//check croping rectangle exceed image boundary
	for (int i = 0; i < 2; i++) {
		if (cropRect[i].width > src.cols - cropRect[i].x)
			cropRect[i].width = src.cols - cropRect[i].x;

		if (cropRect[i].height > src.rows - cropRect[i].y)
			cropRect[i].height = src.rows - cropRect[i].y;

		if (cropRect[i].x < 0)
			cropRect[i].x = 0;

		if (cropRect[i].y < 0)
			cropRect[i].height = 0;
	}
}
unsigned short Configure::getZeroReference(Mat initDepthFrame) {
	unsigned short maxVal = 1150;
	Mat blurMat = initDepthFrame.clone();
	medianBlur(blurMat, blurMat, 5);

	double vmin, vmax;
	int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };

	minMaxIdx(blurMat, &vmin, &vmax, idx_min, idx_max);
	printf("MaxVal: %5.0f \n"
		, vmax);
	maxVal = unsigned short(vmax);
	return maxVal;
}



