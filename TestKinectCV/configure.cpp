#include "configureApp.h"

Configure::Configure(Rect crpRct, Point pnt1, Point pnt2)
{
	cropRect = crpRct;
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

	/*case  CV_EVENT_MOUSEMOVE:
		if (released) {
			P2.x = x;
			P2.y = y;
		}
		break;*/

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
			cropRect = tempRect;
			printf("Interaction area set as X: %d Y: %d W: %d H: %d \n", cropRect.x, cropRect.y, cropRect.width, cropRect.height);
			displayAreaSet = true;
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
	char c;
	do {
		setMouseCallback("Choose interaction area then highlight dead pixels.", onMouse, this);
		imshow("Choose interaction area then highlight dead pixels.", capturedImage);
		
	} while (cvWaitKey(0) != 27);
	

}
void Configure::applyConfigurationSettingsToMatrix(Mat& src) {
	Mat img;
	Mat ROI;

	img = src.clone();
	checkBoundary(src);
	
	for (int i = 0; i < rectangles.size(); i++)
	{
		if (rectangles[i].width>0 && rectangles[i].height>0) {
			//GaussianBlur(src(rectangles[i]), src(rectangles[i]), Size(0,0), 4);
			medianBlur(src(rectangles[i]), src(rectangles[i]), 5);
		}
	}
	if (cropRect.width>0 && cropRect.height>0) {
		ROI = src(cropRect);

	}

	//rectangle(img, cropRect, Scalar(0, 255, 0), 3, 8, 0);
	src = ROI;
}

void Configure::checkBoundary(Mat& src) {
	//check croping rectangle exceed image boundary
	if (cropRect.width>src.cols - cropRect.x)
		cropRect.width = src.cols - cropRect.x;

	if (cropRect.height>src.rows - cropRect.y)
		cropRect.height = src.rows - cropRect.y;

	if (cropRect.x<0)
		cropRect.x = 0;

	if (cropRect.y<0)
		cropRect.height = 0;
}