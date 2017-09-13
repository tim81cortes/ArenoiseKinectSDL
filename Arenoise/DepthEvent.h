#include <queue>
#include <string>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>


namespace dpthEvent {
	enum eventType : unsigned char { evnt_Trigger = 1, evnt_Toggle,  evnt_continuous, evnt_persistent, evnt_End};
};
	// TODO consider making unsigned short as the maximum expected coordinate value needed.
	struct _3dCoordinates {
		double values[3];
	};
	struct orientationVector{
		unsigned short center[2];
		unsigned short front[2];
		unsigned short side[2];
		double distFromCentre[2];
	};

class DepthEvent {



private:
	std::string eventKey;
	unsigned char eventType = 0;
	bool finished = false;
	cv::Mat temp;
	std::queue<orientationVector> orVectQ;
	std::queue<_3dCoordinates> _3dCoordQ;
	std::queue<unsigned short> continuousData;
	unsigned short index = 0;
	
public:
	DepthEvent(std::string evntKey, unsigned char evntTyp, cv::Mat& dataSrc);
	DepthEvent(std::string evntKey, unsigned char eventTyp);
	DepthEvent(std::string evntKey, unsigned char eventTyp, cv::Mat& dataSrc, _3dCoordinates* _3dCrds, unsigned char len);
	DepthEvent(std::string evntKey, unsigned char eventTyp, orientationVector ortnVctr, unsigned char len);
	DepthEvent(std::string evntKey, unsigned char eventTyp, _3dCoordinates coords, unsigned char len);
	//TODO void enqueueContinuousData();
	//TODO void dequeueContinuousData();
	void endToggle(std::string evntKey);
	std::string getEventName();
	unsigned char getEventType();
	_3dCoordinates getCoordinates();
	orientationVector getOrientationVector();
};