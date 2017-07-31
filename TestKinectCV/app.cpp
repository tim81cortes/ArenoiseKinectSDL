#include "App.h"	
#include "synthTone.h"
#include <algorithm>

void App::Init()
{	
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
		
		//depthBufferUpdatedSurface = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];
		depthBufferCurrentDepthFrame = new uint16[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];

		while(!getFrame());
		config = new Configure(Rect(30,30,30,30),Point(0,0),Point(0,0));
		
		Mat mat2(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);
		Mat NormMat;
		Mat DisplayMat;
		Mat imgCropped;
		cv::normalize(mat2, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
		NormMat.convertTo(DisplayMat, CV_8UC3);
		config->defineRegions(DisplayMat);
		// Set flag to say that subsequent frames should be sent to different array
		initFrameDone = true;
		mat2.copyTo(updatedSurface);
		SafeRelease(depthFrame);
		cv::namedWindow("CvOutput", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		cv::namedWindow("CvOutput2", CV_WINDOW_NORMAL);
		cvSetWindowProperty("CvOutput2", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	}
}
void App::flipAndDisplay(Mat& toFlip, const String window, int wait)
{
	cv::Mat toShow;
	cv::flip(toFlip, toShow, 1);
	cv::imshow(window, toShow);
	cv::waitKey(wait);
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
	if (true == foundSensor)
	{
		getFrame();
		Mat depthMatOriginal(DEPTHMAPHEIGHT, DEPTHMAPWIDTH, CV_16U, depthBufferCurrentDepthFrame);		
		
		for (int j = 0; j < depthMatOriginal.rows; j++)
		{
			for (int i = 0; i < depthMatOriginal.cols; i++)
			{
				if (depthMatOriginal.at<uint16>(j, i) > 1250)
				{
					depthMatOriginal.at<uint16>(j, i) = 0;

				}
				else
				{
					if (depthMatOriginal.at<uint16>(j, i) > 1000 && depthMatOriginal.at<uint16>(j, i) != 0 && !updatedSurface.empty())
					{
						updatedSurface.at<uint16>(j, i) = depthMatOriginal.at<uint16>(j, i);
					}
				}

			}
		}
		//printf("Pixel val: %d \n", mat.at<uint16>(200,200));
		Mat matCropped(config->cropRect.height, config->cropRect.width, CV_16U);
		Mat Mat2Cropped = updatedSurface.clone();	
		medianBlur(depthMatOriginal.clone(), matCropped, 5); //TODO remove .clone() if possible
		//medianBlur(updatedSurface.clone(), Mat2Cropped, 5); //TODO remove .clone() if possible

		config->applyConfigurationSettingsToMatrix(matCropped);

		for (int j = 0; j < matCropped.rows; j++)
		{
			for (int i = 0; i < matCropped.cols; i++)
			{

			}
		}


		double vmin, vmax;
		int idx_min[2] = { 255,255 }, idx_max[2] = { 255, 255 };

		minMaxIdx(matCropped, &vmin, &vmax, idx_min, idx_max);
		//printf("MinVal: %f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
			//, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);

		cv::Mat NormMat;
		cv::Mat FlippedMat;
		cv::Mat ScalledMat;
		cv::Mat BeforeColouredMat;
		cv::Mat BeforeInvertedMat;
		cv::Mat ThresholdedMat;
		Mat PlottedMat;
		//cv::threshold(mat, ThresholdedMat, 255.0, 0 ,THRESH_TOZERO);
		config->applyConfigurationSettingsToMatrix(depthMatOriginal);
		cv::normalize(depthMatOriginal, NormMat, 0, 255, CV_MINMAX, CV_16UC1);
		int colmap = 4;
		cv::Mat DisplayMat;
		NormMat.convertTo(BeforeColouredMat, CV_8UC3);
		cv::applyColorMap(BeforeColouredMat, BeforeInvertedMat, colmap);
		cv::bitwise_not(BeforeInvertedMat, DisplayMat);
		
		

		// Second time arround with updating sand surface
		
		/*medianBlur(updatedSurface.clone(), Mat2Cropped, 5);
		config->showImage(Mat2Cropped, config->cropRect);*/
		
		//printf("MinVal: %f ; MaxVal: %5.0f; MinX: %d MinY: %d; MaxX: %d MaxY %d   \n"
		//, vmin, vmax, idx_min[1], idx_min[0], idx_max[1], idx_max[0]);

		cv::Mat NormMat2;
		cv::Mat FlippedMat2;
		cv::Mat ScalledMat2;
		cv::Mat BeforeColouredMat2;
		cv::Mat BeforeInvertedMat2;
		cv::Mat ThresholdedMat2;
		Mat PlottedMat2;
		//cv::threshold(depthMatOriginal, ThresholdedMat2, 255.0, 0 ,THRESH_TOZERO);
		config->applyConfigurationSettingsToMatrix(Mat2Cropped);
		cv::normalize(Mat2Cropped, NormMat2, 0, 255, CV_MINMAX, CV_16UC1);

		cv::Mat DisplayMat2;
		
		NormMat2.convertTo(BeforeColouredMat2, CV_8UC3);
		cv::applyColorMap(BeforeColouredMat2, DisplayMat2, colmap + 5);
		//cv::bitwise_not(BeforeInvertedMat2, DisplayMat2);
		
		
		
		// scale the values to map to the parameter they control
		uint16 pitchVal = 48 + floor(double(35) / matCropped.cols * idx_max[1]);
		uint16 lpfVal = 1000 + floor(double(2000) / matCropped.cols * idx_max[0]); 

		// Send OSC
		//printf("Columns: %d Rows: %d Pitch: %d CutoffFrequency: %d \n", matCropped.cols, matCropped.rows, pitchVal, lpfVal);
		// Only send a message when the changes occur near the surface
		// Exclude changes far above and below the surface
		if (vmax < 1950 && vmin > 1000) 
		{
			if (abs(vmin - currentMin) > 30) 
			{
				try 
				{
					printf("Triggering OSC send. Pitch: %d Cuttoff %d ", pitchVal, lpfVal);
					outBoundPS	<< osc::BeginMessage("/t") << pitchVal << lpfVal << osc::EndMessage;
					trnsmtSock.Send(outBoundPS.Data(), outBoundPS.Size());
					outBoundPS.Clear();
				}
				catch(Exception e)
				{
					printf("Error: %s %s", e.err, e.msg);
				}
			}
		}
		currentMin = vmin;		  
		flipAndDisplay(DisplayMat, "CvOutput", 2);
		flipAndDisplay(DisplayMat2, "CvOutput2", 2);

		SafeRelease(depthFrame);
	}
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
}