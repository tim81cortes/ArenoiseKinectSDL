#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

using namespace cv;
//using namespace osc;


class Configure {

private:
	
	Point P1;
	Point P2;
	bool clicked;

public:
	Rect cropRect;
	Configure(Rect crpRct, Point pnt1, Point pnt2);
	//~Configure();
	void onMouse(int event, int x, int y);
	static void onMouse(int event, int x, int y, int, void* userdata);
	void cropWindow(Mat& capturedImage);
	void showImage(Mat& src, Rect crpdRct);
	void saveImage(Mat& src, Rect crpdRct);
	void checkBoundary(Mat& src);
};

