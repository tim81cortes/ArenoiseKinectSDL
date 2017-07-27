#include "configureApp.h"

Configure::Configure(Rect crpRct, Point pnt1, Point pnt2)
{
	cropRect = crpRct;
	P1 = pnt1;
	P2 = pnt2;
}

//void Configure::onMouse(int event, int x, int y) {
//
//}

void Configure::onMouse(int event, int x, int y) {
	switch (event) {

	case  CV_EVENT_LBUTTONDOWN:
		clicked = true;

		P1.x = x;
		P1.y = y;
		P2.x = x;
		P2.y = y;
		break;

	case  CV_EVENT_LBUTTONUP:
		P2.x = x;
		P2.y = y;
		clicked = false;
		break;

	case  CV_EVENT_MOUSEMOVE:
		if (clicked) {
			P2.x = x;
			P2.y = y;
		}
		break;

	default:   break;


	}

	if (clicked) {
		if (P1.x>P2.x) {
			cropRect.x = P2.x;
			cropRect.width = P1.x - P2.x;
		}
		else {
			cropRect.x = P1.x;
			cropRect.width = P2.x - P1.x;
		}

		if (P1.y>P2.y) {
			cropRect.y = P2.y;
			cropRect.height = P1.y - P2.y;
		}
		else {
			cropRect.y = P1.y;
			cropRect.height = P2.y - P1.y;
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

void Configure::cropWindow(Mat& capturedImage) {
	Mat imgOriginal;        // input image
	Mat imgCropped;			// image after cropping
	Mat imgGrayscale;       // grayscale of input image
	Mat imgBlurred;         // intermediate blured image
	Mat imgCanny;           // Canny edge image
	
	namedWindow("WhichArea", CV_WINDOW_NORMAL);
	//namedWindow("cropped", CV_WINDOW_NORMAL);
	setMouseCallback("WhichArea", onMouse, this);
	imshow("WhichArea", capturedImage);
	waitKey(0); // show windows
	//imgCropped = capturedImage.clone();
	//capturedImage = imgCropped;

}
void Configure::showImage(Mat& src, Rect crpdRct) {
	Mat img;
	Mat ROI;

	img = src.clone();
	checkBoundary(src);
	if (crpdRct.width>0 && crpdRct.height>0) {
		ROI = src(crpdRct);
		//imshow("cropped", ROI);
	}

	rectangle(img, crpdRct, Scalar(0, 255, 0), 3, 8, 0);
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