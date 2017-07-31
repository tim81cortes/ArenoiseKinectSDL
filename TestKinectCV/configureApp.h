#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <iostream>

using namespace cv;


class Configure {

private:
	Point P1;
	Point P2;
	bool justReleased = false;
	bool wasClicked = false;
	// The first 1 rectangles is responsible for defining the interaction area of the screen.
	// The rest are used to manage dead pixels
	bool displayAreaSet = false;
	std::vector<Rect> rectangles;

public:
	Rect cropRect;
	Configure(Rect crpRct, Point pnt1, Point pnt2);
	//~Configure();
	void onMouse(int event, int x, int y);
	static void onMouse(int event, int x, int y, int, void* userdata);
	void defineRegions(Mat& capturedImage);
	void applyConfigurationSettingsToMatrix(Mat& src);
	void checkBoundary(Mat& src);
	unsigned short loadConfigurationData(String depthFrameName);
	unsigned short boxBottom = 1150;
};

