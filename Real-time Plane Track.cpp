// E11.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;



class FeatureTracker{
	Mat gray;					//当前帧灰度图
	Mat gray_prev;				//上一帧的灰度图
	vector<Point2f> points[2];	//前后两帧的特征点
	vector<uchar> status;		//特征跟踪状态
	vector<float> err;			//跟踪时的错误
public:
	FeatureTracker(){}
	void init(vector<Point2f> init) { 
		points[0].clear();
		points[0].insert(points[0].end(), init.begin(), init.end());
	}
	vector<Point2f> process(Mat &frame, Mat &output) {
		//得到灰度图
		cvtColor(frame, gray, COLOR_BGR2GRAY);
		frame.copyTo(output);
		//第一帧
		if (gray_prev.empty()) {
			gray.copyTo(gray_prev);
		}
		calcOpticalFlowPyrLK(
			gray_prev,		//前一帧灰度图
			gray,			//当前帧灰度图
			points[0],		//前一帧特征点位置
			points[1],		//当前帧特征点位置
			status,			//特征点被成功跟踪的标志
			err);			//前一帧特征点小区域和当前特征点小区域间的差，根据差的大小可删除那些运动变化剧烈的点
		//更新上一帧
		std::swap(points[1], points[0]);
		cv::swap(gray_prev, gray);
		return points[0];
	}


	bool getStatus(int i)
	{
		return status[i];
	}
	int getSize()
	{
		return status.size();
	}

};


void KLT()
{

	Mat tmp = imread("C:\\Users\\70922\\Desktop\\CV\\E12\\t0.jpg");

	VideoCapture cap(0);
	//VideoWriter writer;

	int w = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
	int h = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));

	Size S(w, h);

	double r = cap.get(CAP_PROP_FPS);

	//writer.open("C:\\Users\\70922\\Desktop\\CV\\E12\\1.avi", writer.fourcc('M', 'J', 'P', 'G'), r, S, true);

	if (!cap.isOpened())
	{
		return;
	}
	bool stop = false;
	Mat frame;
	FeatureTracker tracker;
	vector<DMatch> goodMatches;
	vector<KeyPoint> keypoints1, keypoints2;
	vector<DMatch> matchPoints;
	vector<KeyPoint> RR_keypoint1, RR_keypoint2;
	vector<DMatch> RR_matches;
	int idx = 0;
	vector<Point2f> init;


	while (!stop)
	{
		if (!cap.read(frame))
			return;
		Mat img1, img2;
		tmp.copyTo(img1);
		frame.copyTo(img2);
		
		Ptr<SIFT> sift = SIFT::create(100);

		Mat descriptors1, descriptors2, outframe;

		//第一帧
		if (idx < 15)
		{
			keypoints1.clear();
			keypoints2.clear();
			init.clear();
			double t1 = (double)getTickCount();
			sift->detectAndCompute(img1, Mat(), keypoints1, descriptors1);
			sift->detectAndCompute(img2, Mat(), keypoints2, descriptors2);
			

			//特征点匹配
			BFMatcher matcher;
			matchPoints.clear();
			matcher.match(descriptors1, descriptors2, matchPoints, Mat());

			////找出最优特征点
			//double minDist = 1000;    //初始化最大最小距离
			//double maxDist = 0;

			//for (int i = 0; i < descriptors1.rows; i++)
			//{
			//	double dist = matchPoints[i].distance;
			//	if (dist > maxDist)
			//	{
			//		maxDist = dist;
			//	}
			//	if (dist < minDist)
			//	{
			//		minDist = dist;
			//	}
			//}

			//
			//goodMatches.clear();
			//for (int i = 0; i < descriptors1.rows; i++)
			//{
			//	double dist = matchPoints[i].distance;
			//	if (dist < max(2 * minDist, 0.02)) {
			//		goodMatches.push_back(matchPoints[i]);
			//	}
			//}
			//t1 = ((double)getTickCount() - t1) / getTickFrequency();


			//Mat dst;
			//drawMatches(img1, keypoints1, img2, keypoints2, goodMatches, dst,
			//	Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);


			//根据goodMatches将特征点对齐,将坐标转换为float类型
			vector<KeyPoint> R_keypoint1, R_keypoint2;
			for (size_t i = 0; i < matchPoints.size(); i++)
			{
				R_keypoint1.push_back(keypoints1[matchPoints[i].queryIdx]);
				R_keypoint2.push_back(keypoints2[matchPoints[i].trainIdx]);
			}

			//坐标转换
			vector<Point2f>p1, p2;
			for (size_t i = 0; i < matchPoints.size(); i++)
			{
				p1.push_back(R_keypoint1[i].pt);
				p2.push_back(R_keypoint2[i].pt);
			}

			//利用基础矩阵剔除误匹配点
			vector<uchar> RansacStatus;
			Mat Fundamental = findFundamentalMat(p1, p2, RansacStatus, FM_RANSAC);

			//重新定义RR_keypoint和RR_matches来存储新的关键点和匹配矩阵
			RR_keypoint1.clear();
			RR_keypoint2.clear();
			RR_matches.clear();
			int index = 0;
			for (size_t i = 0; i < matchPoints.size(); i++)
			{
				if (RansacStatus[i] != 0)
				{
					RR_keypoint1.push_back(R_keypoint1[i]);
					RR_keypoint2.push_back(R_keypoint2[i]);
					matchPoints[i].queryIdx = index;
					matchPoints[i].trainIdx = index;
					RR_matches.push_back(matchPoints[i]);
					index++;
				}
			}

			for (int i = 0; i < RR_keypoint2.size(); i++) {
				init.push_back(RR_keypoint2[i].pt);
			}

			tracker.init(init);


			t1 = ((double)getTickCount() - t1) / getTickFrequency();
			Mat dst;
			drawMatches(img1, RR_keypoint1, img2, RR_keypoint2, RR_matches, dst,
				Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);


			double fps = 1.0 / t1;
			char str[10];

			sprintf_s(str, "%.2f", fps);      // 帧率保留两位小数
			string fpsString("FPS:");
			fpsString += str;                    // 在"FPS:"后加入帧率数值字符串
			// 将帧率信息写在输出帧上
			putText(dst,
				fpsString,
				Point(20, 20),
				FONT_HERSHEY_PLAIN,
				2, // 字体大小
				Scalar(0, 255, 0),
				2);
			cout << "SIFT:" << fps << endl;
			imshow("KLT", dst);
			if (matchPoints.size()>0)idx++;
			//if (idx == 15)myMatches = goodMatches;
			
			
		}
		else {
			
			double t1 = (double)getTickCount();
			vector<KeyPoint> kpt2;
			vector<Point2f> points = tracker.process(img2, outframe);
			KeyPoint::convert(points, kpt2, 1, 1, 0, -1);
			vector<KeyPoint> new_keypoint1, new_keypoint2;
			vector<DMatch> KLT_matches;
			int index = 0;
			
			for (size_t i = 0; i < RR_matches.size(); i++)
			{
				//cout << tracker.getSize() << endl;
				if (tracker.getStatus(i))
				{
					//printf("ok\n");
					new_keypoint1.push_back(RR_keypoint1[i]);
					new_keypoint2.push_back(kpt2[i]);
					
					RR_matches[i].queryIdx = index;
					RR_matches[i].trainIdx = index;
					KLT_matches.push_back(RR_matches[i]);
					index++;
				}
			}

			t1 = ((double)getTickCount() - t1) / getTickFrequency();
			Mat dst;
			drawMatches(img1, new_keypoint1, img2, new_keypoint2, KLT_matches, dst,
				Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

			double fps = 1.0 / t1;
			char str[10];

			sprintf_s(str, "%.2f", fps);      // 帧率保留两位小数
			string fpsString("FPS:");
			fpsString += str;                // 在"FPS:"后加入帧率数值字符串
			// 将帧率信息写在输出帧上
			putText(dst,
				fpsString,
				Point(20, 20),
				FONT_HERSHEY_PLAIN,
				2, // 字体大小
				Scalar(0, 255, 0),
				2);
			cout << "KLT:" << fps << endl;
			imshow("KLT", dst);

		}
		
		

		if (cv::waitKey(27) > 0)
		{
			stop = true;
		}
		if (cv::waitKey(82) > 0)
		{
			idx = 0;
		}
	}
	cap.release();
}



Mat mySIFT(Mat& img1, Mat& img2, int nfeature = 100)
{
	Mat image1, image2;
	GaussianBlur(img1, image1, Size(3, 3), 0.5, 0.5);
	GaussianBlur(img2, image2, Size(3, 3), 0.5, 0.5);

	//定义SIFT特征检测器
	Ptr<SIFT> sift = SIFT::create(nfeature);

	//检测特征点，计算特征点描述子
	double t1 = (double)getTickCount();
	vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;
	sift->detectAndCompute(img1, Mat(), keypoints1, descriptors1);
	sift->detectAndCompute(img2, Mat(), keypoints2, descriptors2);
	

	//特征点匹配
	BFMatcher matcher;
	vector<DMatch> matchPoints;
	matcher.match(descriptors1, descriptors2, matchPoints, Mat());


	//最近邻匹配FLANN
	//vector<DMatch> matchPoints;
	//FlannBasedMatcher matcher;
	//matcher.match(descriptors1, descriptors2, matchPoints);

	//找出最优特征点
	double minDist = 1000;    //初始化最大最小距离
	double maxDist = 0;

	for (int i = 0; i < descriptors1.rows; i++)
	{
		double dist = matchPoints[i].distance;
		if (dist > maxDist)
		{
			maxDist = dist;
		}
		if (dist < minDist)
		{
			minDist = dist;
		}
	}

	vector<DMatch> goodMatches;
	for (int i = 0; i < descriptors1.rows; i++)
	{
		double dist = matchPoints[i].distance;
		if (dist < max(2 * minDist, 0.02)) {
			goodMatches.push_back(matchPoints[i]);
		}
	}
	t1 = ((double)getTickCount() - t1) / getTickFrequency();


	Mat dst;
	drawMatches(img1, keypoints1, img2, keypoints2, goodMatches, dst,
		Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	//根据matchPoints将特征点对齐,将坐标转换为float类型
	//vector<KeyPoint> R_keypoint1, R_keypoint2;
	//for (size_t i = 0; i < matchPoints.size(); i++)
	//{
	//	R_keypoint1.push_back(keypoints1[matchPoints[i].queryIdx]);
	//	R_keypoint2.push_back(keypoints2[matchPoints[i].trainIdx]);
	//}

	////坐标转换
	//vector<Point2f>p1, p2;
	//for (size_t i = 0; i < matchPoints.size(); i++)
	//{
	//	p1.push_back(R_keypoint1[i].pt);
	//	p2.push_back(R_keypoint2[i].pt);
	//}

	////利用基础矩阵剔除误匹配点
	//vector<uchar> RansacStatus;
	//Mat Fundamental = findFundamentalMat(p1, p2, RansacStatus, FM_RANSAC, 3.0, 0.995);

	////重新定义RR_keypoint和RR_matches来存储新的关键点和匹配矩阵
	//vector<KeyPoint> RR_keypoint1, RR_keypoint2;
	//vector<DMatch> RR_matches;     
	//int index = 0;
	//for (int i = 0; i < matchPoints.size()&& i < RansacStatus.size(); i++)
	//{
	//	if (RansacStatus[i] != 0)
	//	{
	//		RR_keypoint1.push_back(R_keypoint1[i]);
	//		RR_keypoint2.push_back(R_keypoint2[i]);
	//		matchPoints[i].queryIdx = index;
	//		matchPoints[i].trainIdx = index;
	//		RR_matches.push_back(matchPoints[i]);
	//		index++;
	//	}
	//}
	//t1 = ((double)getTickCount() - t1) / getTickFrequency();

	//Mat dst;
	//drawMatches(img1, RR_keypoint1, img2, RR_keypoint2, RR_matches, dst,
	//	Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	//imshow("After RANSAC Image", img_RR_matches);

	double fps = 1.0 / t1;
	char str[10];

	sprintf_s(str, "%.2f", fps);      // 帧率保留两位小数
	string fpsString("FPS:");
	fpsString += str;                    // 在"FPS:"后加入帧率数值字符串
	// 将帧率信息写在输出帧上
	putText(dst,
		fpsString,
		Point(20, 20),
		FONT_HERSHEY_PLAIN,
		2, // 字体大小
		Scalar(0, 255, 0),
		2);
	return dst;

}

Mat mySURF(Mat &img1, Mat &img2, int minHessian = 3000)
{
	Mat image1 = img1.clone(), image2 = img2.clone();
	GaussianBlur(img1, image1, Size(3, 3), 0.5, 0.5);
	GaussianBlur(img2, image2, Size(3, 3), 0.5, 0.5);

	//定义SURF特征检测器
	Ptr<SURF> surf = SURF::create(minHessian);
	double t1 = (double)getTickCount();
	vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;
	surf->detectAndCompute(img1, Mat(), keypoints1, descriptors1);
	surf->detectAndCompute(img2, Mat(), keypoints2, descriptors2);
	

	//特征点匹配
	BFMatcher matcher;
	vector<DMatch> matchPoints;
	matcher.match(descriptors1, descriptors2, matchPoints, Mat());



	//最近邻匹配FLANN
	//vector<DMatch> matchPoints;
	//FlannBasedMatcher matcher;
	//matcher.match(descriptors1, descriptors2, matchPoints);


	//找出最优特征点
	//double minDist = 1000;    //初始化最大最小距离
	//double maxDist = 0;

	//for (int i = 0; i < descriptors1.rows; i++)
	//{
	//	double dist = matchPoints[i].distance;
	//	if (dist > maxDist)
	//	{
	//		maxDist = dist;
	//	}
	//	if (dist < minDist)
	//	{
	//		minDist = dist;
	//	}
	//}
	//printf("maxDist:%f\n", maxDist);
	//printf("minDist:%f\n", minDist);

	//vector<DMatch> goodMatches;
	//for (int i = 0; i < descriptors1.rows; i++)
	//{
	//	double dist = matchPoints[i].distance;
	//	if (dist < max(2 * minDist, 0.02)) {
	//		goodMatches.push_back(matchPoints[i]);
	//	}
	//}
	//t1 = ((double)getTickCount() - t1) / getTickFrequency();
	//Mat dst;
	//drawMatches(img1, keypoints1, img2, keypoints2, goodMatches, dst,
	//	Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	//

	//根据goodMatches将特征点对齐,将坐标转换为float类型
	vector<KeyPoint> R_keypoint1, R_keypoint2;
	for (size_t i = 0; i < matchPoints.size(); i++)
	{
		R_keypoint1.push_back(keypoints1[matchPoints[i].queryIdx]);
		R_keypoint2.push_back(keypoints2[matchPoints[i].trainIdx]);
	}

	//坐标转换
	vector<Point2f>p1, p2;
	for (size_t i = 0; i < matchPoints.size(); i++)
	{
		p1.push_back(R_keypoint1[i].pt);
		p2.push_back(R_keypoint2[i].pt);
	}

	//利用基础矩阵剔除误匹配点
	vector<uchar> RansacStatus;
	Mat Fundamental = findFundamentalMat(p1, p2, RansacStatus, FM_RANSAC);

	//重新定义RR_keypoint和RR_matches来存储新的关键点和匹配矩阵
	vector<KeyPoint> RR_keypoint1, RR_keypoint2;
	vector<DMatch> RR_matches;     
	int index = 0;
	for (size_t i = 0; i < matchPoints.size(); i++)
	{
		if (RansacStatus[i] != 0)
		{
			RR_keypoint1.push_back(R_keypoint1[i]);
			RR_keypoint2.push_back(R_keypoint2[i]);
			matchPoints[i].queryIdx = index;
			matchPoints[i].trainIdx = index;
			RR_matches.push_back(matchPoints[i]);
			index++;
		}
	}
	t1 = ((double)getTickCount() - t1) / getTickFrequency();
	Mat dst;
	drawMatches(img1, RR_keypoint1, img2, RR_keypoint2, RR_matches, dst,
		Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	//imshow("After RANSAC Image", img_RR_matches);

	double fps = 1.0 / t1;
	char str[10];

	sprintf_s(str, "%.2f", fps);      // 帧率保留两位小数
	string fpsString("FPS:");
	fpsString += str;                    // 在"FPS:"后加入帧率数值字符串
	// 将帧率信息写在输出帧上
	putText(dst,
		fpsString,
		Point(20, 20),
		FONT_HERSHEY_PLAIN,
		2, // 字体大小
		Scalar(0, 255, 0),
		2);
	return dst;


}

Mat myORB(Mat &img1, Mat &img2, int nfeature = 100)
{
	Mat image1 = img1.clone(), image2 = img2.clone();
	GaussianBlur(img1, image1, Size(3, 3), 0.5, 0.5);
	GaussianBlur(img2, image2, Size(3, 3), 0.5, 0.5);
	double t1 = (double)getTickCount();
	//定义ORB特征检测器
	Ptr<ORB> orb = ORB::create(nfeature);
	vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;
	orb->detectAndCompute(image1, Mat(), keypoints1, descriptors1);
	orb->detectAndCompute(image2, Mat(), keypoints2, descriptors2);


	//暴力匹配  使用 Hamming 距离
	vector<DMatch> matchPoints;
	BFMatcher matcher(NORM_HAMMING);
	matcher.match(descriptors1, descriptors2, matchPoints);

	//最近邻匹配FLANN
	//vector<DMatch> matchPoints;
	//FlannBasedMatcher matcher(new flann::LshIndexParams(20, 10, 2));
	//matcher.match(descriptors1, descriptors2, matchPoints);

	//找出最优特征点
	//double minDist = 1000;    //初始化最大最小距离
	//double maxDist = 0;

	//for (int i = 0; i < descriptors1.rows; i++)
	//{
	//	double dist = matchPoints[i].distance;
	//	if (dist > maxDist)
	//	{
	//		maxDist = dist;
	//	}
	//	if (dist < minDist)
	//	{
	//		minDist = dist;
	//	}
	//}

	//vector<DMatch> goodMatches;
	//for (int i = 0; i < descriptors1.rows; i++)
	//{
	//	double dist = matchPoints[i].distance;
	//	//printf("distance:%f", matchPoints[i].distance);
	//	if (dist < max(2 * minDist, 0.02)) {
	//		goodMatches.push_back(matchPoints[i]);
	//	}
	//}
	//t1 = ((double)getTickCount() - t1) / getTickFrequency();
	//Mat dst;
	//drawMatches(img1, keypoints1, img2, keypoints2, matchPoints, dst,
	//	Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);


	//根据goodMatches将特征点对齐,将坐标转换为float类型
	vector<KeyPoint> R_keypoint1, R_keypoint2;
	for (size_t i = 0; i < matchPoints.size(); i++)
	{
		R_keypoint1.push_back(keypoints1[matchPoints[i].queryIdx]);
		R_keypoint2.push_back(keypoints2[matchPoints[i].trainIdx]);
	}

	//坐标转换
	vector<Point2f>p1, p2;
	for (size_t i = 0; i < matchPoints.size(); i++)
	{
		p1.push_back(R_keypoint1[i].pt);
		p2.push_back(R_keypoint2[i].pt);
	}

	//利用基础矩阵剔除误匹配点
	vector<uchar> RansacStatus;
	Mat Fundamental = findFundamentalMat(p1, p2, RansacStatus, FM_RANSAC);

	//重新定义RR_keypoint和RR_matches来存储新的关键点和匹配矩阵
	vector<KeyPoint> RR_keypoint1, RR_keypoint2;
	vector<DMatch> RR_matches;
	int index = 0;
	for (size_t i = 0; i < matchPoints.size(); i++)
	{
		if (RansacStatus[i] != 0)
		{
			RR_keypoint1.push_back(R_keypoint1[i]);
			RR_keypoint2.push_back(R_keypoint2[i]);
			matchPoints[i].queryIdx = index;
			matchPoints[i].trainIdx = index;
			RR_matches.push_back(matchPoints[i]);
			index++;
		}
	}
	t1 = ((double)getTickCount() - t1) / getTickFrequency();
	Mat dst;
	drawMatches(img1, RR_keypoint1, img2, RR_keypoint2, RR_matches, dst,
		Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	double fps = 1.0 / t1;
	char str[10];

	sprintf_s(str, "%.2f", fps);      // 帧率保留两位小数
	string fpsString("FPS:");
	fpsString += str;                    // 在"FPS:"后加入帧率数值字符串
	// 将帧率信息写在输出帧上
	putText(dst,
		fpsString,
		Point(20, 20),
		FONT_HERSHEY_PLAIN,
		2, // 字体大小
		Scalar(0, 255, 0),
		2);

	return dst;
}


int main()
{
	//SIFT SURF ORB
	
	/*
	Mat tmp = imread("C:\\Users\\70922\\Desktop\\CV\\E12\\t6.jpg");

	VideoCapture cap(0);
	//VideoWriter writer;

	int w = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
	int h = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
	Size S(w, h);
	double r = cap.get(CAP_PROP_FPS);
	//pyrDown(tmp, tmp);
	//writer.open("C:\\Users\\70922\\Desktop\\CV\\E12\\1.avi", writer.fourcc('M', 'J', 'P', 'G'), r, S, true);

	if (!cap.isOpened())
	{
		return 1;
	}
	bool stop = false;
	Mat frame, down_frame;
	while (!stop)
	{
		//读取帧
		if (!cap.read(frame))
			break;
		//pyrDown(frame, down_frame);
		//resize(frame, down_frame, Size(480, 360));
		Mat dst = myORB(tmp, frame);
		

		imshow("ORB", dst);
		//写入文件
		//writer.write(dst);
		if (cv::waitKey(27) > 0)
		{
			stop = true;
		}
	}
	//释放对象
	cap.release();
	//writer.release();

	*/
	

	//光流法
	KLT();


	return 0;

}



/*
void cv::calcOpticalFlowPyrLK(
	InputArray 	prevImg,				// buildOpticalFlowPyramid构造的第一个8位输入图像或金字塔。
	InputArray 	nextImg,				// 与prevImg相同大小和相同类型的第二个输入图像或金字塔
	InputArray 	prevPts,				// 需要找到流的2D点的矢量，点坐标必须是单精度浮点数。
	InputOutputArray 	nextPts,		// 输出二维点的矢量（具有单精度浮点坐标），包含第二图像中输入特征的计算新位置;
										// 当传递OPTFLOW_USE_INITIAL_FLOW标志时，向量必须与输入中的大小相同。
	OutputArray 	status,				// 输出状态向量（无符号字符）,如果找到相应特征的流，则向量的每个元素设置为1，否则设置为0。
	OutputArray 	err,				// 输出错误的矢量; 向量的每个元素都设置为相应特征的错误，错误度量的类型可以在flags参数中设置; 如果未找到流，则未定义错误
	Size 	winSize = Size(21, 21),		// 每个金字塔等级的搜索窗口的winSize大小
	int 	maxLevel = 3,				// 基于0的最大金字塔等级数;如果设置为0，则不使用金字塔（单级），如果设置为1，则使用两个级别，依此类推
	TermCriteria 	criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 30, 0.01), //指定迭代搜索算法的终止条件
	int 	flags = 0,					// 操作标志: 1. OPTFLOW_USE_INITIAL_FLOW使用初始估计;2. OPTFLOW_LK_GET_MIN_EIGENVALS使用最小特征值作为误差测量
	double 	minEigThreshold = 1e-4		// 算法计算光流方程的2x2正常矩阵的最小特征值，除以窗口中的像素数;如果此值小于minEigThreshold，则过滤掉相应的功能并且不处理其流程，因此它允许删除坏点并获得性能提升。
);

*/










// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
