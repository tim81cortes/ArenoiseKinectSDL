#include "DepthEvent.h"

DepthEvent::DepthEvent(std::string evntKey, unsigned char evntTyp, cv::Mat& dataSrc)
{
	eventType = evntTyp;
	cv::Mat temp = dataSrc;

}

DepthEvent::DepthEvent(std::string evntKey, unsigned char eventTyp)
{
	eventKey = evntKey;
	eventType = eventTyp;
}

DepthEvent::DepthEvent(std::string evntKey, unsigned char eventTyp, cv::Mat & dataSrc, _3dCoordinates * _3dCrds, unsigned char len)
{

}

DepthEvent::DepthEvent(std::string evntKey, unsigned char eventTyp, orientationVector ortnVctr, unsigned char len)
{
	eventKey = evntKey;
	eventType = eventTyp;
	orVectQ.emplace(ortnVctr);
}

DepthEvent::DepthEvent(std::string evntKey, unsigned char eventTyp, _3dCoordinates coords, unsigned char lngth)
{
	eventKey = evntKey;
	eventType = eventTyp;
	_3dCoordQ.emplace(coords);
}

void DepthEvent::endToggle(std::string evntKey)
{
	finished = true;
}

std::string DepthEvent::getEventName()
{
	
	return eventKey;
}

unsigned char DepthEvent::getEventType()
{
	return eventType;
}

_3dCoordinates DepthEvent::getCoordinates()
{
	_3dCoordinates tempCoords = {0,0,0};
	
	if (_3dCoordQ.size() != 0)
	{
		tempCoords = _3dCoordQ.front();
		_3dCoordQ.pop();
	}
	
	return tempCoords;
}

orientationVector DepthEvent::getOrientationVector()
{
	orientationVector tempOrVect;
	tempOrVect = { {0,0}, {0,0}, {0,0}, 0.0, 0.0 };
	
	if (orVectQ.size() != 0)
	{
		tempOrVect = orVectQ.front();
		orVectQ.pop();
	}

	return tempOrVect;
	
}
