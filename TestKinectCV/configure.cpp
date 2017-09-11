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
		Rect tmpRect;
		if (P1.x>P2.x) 
		{
			tmpRect.x = P2.x;
			tmpRect.width = P1.x - P2.x;
		}
		else 
		{
			tmpRect.x = P1.x;
			tmpRect.width = P2.x - P1.x;
		}

		if (P1.y>P2.y) 
		{
			tmpRect.y = P2.y;
			tmpRect.height = P1.y - P2.y;
		}
		else 
		{
			tmpRect.y = P1.y;
			tmpRect.height = P2.y - P1.y;
		}
		if (!displayAreaSet)
		{
			cropRect[0] = tmpRect;
			printf("Interaction area 1 set as X: %d Y: %d W: %d H: %d \n", cropRect[0].x, cropRect[0].y, cropRect[0].width, cropRect[0].height);
			
			int relativeSize = round(double(tmpRect.height)/10);
			Rect tmpRect2(0,  tmpRect.height - relativeSize, tmpRect.width, relativeSize);
			cropRect[2] = tmpRect2;
			printf("Interaction area 1 set as X: %d Y: %d W: %d H: %d \n", 0, tmpRect.height - relativeSize, tmpRect.width, relativeSize);
 			displayAreaSet = true;

		}
		else if(displayAreaSet && !sideOfBoxAreaSet)
		{
			cropRect[1] = tmpRect;
			printf("Interaction area 2 set as X: %d Y: %d W: %d H: %d \n", cropRect[1].x, cropRect[1].y, cropRect[1].width, cropRect[1].height);
			sideOfBoxAreaSet = true;
		}
		else
		{
			rectangles.push_back(tmpRect);
			printf("Rectangle added for blurring. X: %d Y: %d W: %d H: %d \n", tmpRect.x, tmpRect.y, tmpRect.width, tmpRect.height);
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
	bool noExistingConfigFile = loadConfigSettingsFromFile();
	
	
	if (noExistingConfigFile) 
	{
		namedWindow("Choose interaction area then highlight dead pixels.", CV_WINDOW_NORMAL);
		do {
			setMouseCallback("Choose interaction area then highlight dead pixels.", onMouse, this);
			imshow("Choose interaction area then highlight dead pixels.", capturedImage);
		
		} while (cvWaitKey(0) != 27);
		saveConfigSettingsToFile();
	}

	

}
void Configure::applyConfigurationSettingsToMatrix(Mat& src, int whichArea) 
{
	Mat ROI; // Secondary interaction area i.e. the white platform next to the pit
 	checkBoundary(src);
	for (int i = 0; i < rectangles.size(); i++)
	{
		if (rectangles[i].width>0 && rectangles[i].height>0) {
			medianBlur(src(rectangles[i]), src(rectangles[i]), 5);
		}
	}

		if (cropRect[whichArea].width>0 && cropRect[whichArea].height>0) 
		{
			ROI = src(cropRect[whichArea]);
		}
		unsigned char boxBottomAdj = 0;
		if (0 == whichArea)
		{
			boxBottomAdj = 30;
		}

	subtract((boxBottom[whichArea] + boxBottomAdj), ROI, ROI);
	src = ROI;
}



unsigned short Configure::getZeroReferenceFromFile(String depthFrameName) {
	FileStorage fs;
	Mat emptyBoxFromFile;
	String qualifiedDepthFrameName = "ConfigurationFiles\\" + depthFrameName;
	boxBottom[0] = 1150;
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
	return boxBottom[0];
}

unsigned short Configure::getZeroReferenceFromMatrix(Mat src) {
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

unsigned short Configure::calculateTotalDifferenceFromMin(Mat& depthFrame) {
	
	if (!depthFrame.empty())
	{
		return unsigned short(sum(depthFrame)[0]);
	}
	
	return 0;
}

bool Configure::loadConfigSettingsFromFile()
{
	FileStorage fs;
	std::string filename = "ConfigurationFiles\\configSettings.xml";
	FileNode fn = fs["AnomolousReadingsToBlur"];
	try
	{
		FileStorage file(filename, FileStorage::READ);
		file["CropRect0"] >> cropRect[0];
		file["CropRect1"] >> cropRect[1];
		FileNodeIterator it = fn.begin(), it_end = fn.end(); // Go through the node
		for (; it != it_end; ++it)
		{
			std::cout << (std::string)*it << std::endl;
		}
			
	}
	catch (Exception e)
	{
		printf("There was an error loading the configuration file. %s \n", e.msg);
		
	}

	fs.release();


	return true;
}
unsigned short Configure::saveConfigSettingsToFile()
{
	std::string filename = "ConfigurationFiles\\configSettings.xml";
	FileStorage file(filename.c_str(), FileStorage::WRITE);
	for (unsigned short i = 0; i < 2; i++)
	{
		file << "CropRect" + std::to_string(i) << cropRect[i];
	}
	file << "AnomolousReadingsToBlur" << "{";
	for (unsigned short i = 0; i < rectangles.size(); i++)
	{
		file <<  "rect" + std::to_string(i) << rectangles[i];
	}
	file << "}";
	file.release();
	return 0;
}

void Configure::saveImage(Mat& src, int count) {
	Mat img;
	std::string filename = "SandboxDifferenceMap" + std::to_string(count) + ".xml";
	FileStorage file(filename.c_str(), FileStorage::WRITE);

	file << "SandboxDifferenceMap" << src;
	file.release();

}


