//
//
//
//

#include <string>
#include <cassert>

#include "Gui.h"

using namespace cv;
using namespace std;
using namespace pcl;
using namespace pcl::visualization;

//---------------------------------------------------------------------------------------------------------------------
// Static initialization
Gui* Gui::mInstance = nullptr;

//---------------------------------------------------------------------------------------------------------------------
void Gui::init(string _name, StereoCameras& _stereoCameras) {
	assert(mInstance == nullptr);
	mInstance = new Gui(_name, _stereoCameras);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::end() {
	assert(mInstance != nullptr);
	delete mInstance;
}

//---------------------------------------------------------------------------------------------------------------------
Gui * Gui::get() {
	return mInstance;
}

//---------------------------------------------------------------------------------------------------------------------
// Public interface
//---------------------------------------------------------------------------------------------------------------------

void Gui::drawMap(const PointCloud<PointXYZ>::Ptr & _map) {
	clearMap();
	m3dViewer->updatePointCloud(_map, "map");	// Not efficient but fast implementation	
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::clearMap() {
	m3dViewer->removeAllPointClouds(mViewPortMapViewer);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::drawCandidate(const ObjectCandidate & _candidate)
{
	addCluster(_candidate.cloud(), 3, _candidate.R(), _candidate.G(), _candidate.B());
	reprojectCloud(_candidate.cloud(), _candidate.R(), _candidate.G(), _candidate.B());
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::addCluster(const pcl::PointCloud<pcl::PointXYZ>::Ptr & _cluster, unsigned _pointSize, unsigned _r, unsigned _g, unsigned _b) {
	mPcCounter++;
	m3dViewer->addPointCloud<PointXYZRGB>(colorizePointCloud(_cluster, _r, _g, _b),"Cluster_"+to_string(mPcCounter),mViewPortMapViewer);
	m3dViewer->setPointCloudRenderingProperties (PCL_VISUALIZER_POINT_SIZE, _pointSize, "Cluster_"+to_string(mPcCounter),mViewPortMapViewer);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::addPointToPcViewer(const PointCloud<PointXYZ>::Ptr & _cloud, unsigned _pointSize, unsigned _r, unsigned _g, unsigned _b) {
	mPcCounter++;
	m3dViewer->addPointCloud<PointXYZRGB>(colorizePointCloud(_cloud, _r, _g, _b),"Cloud"+to_string(mPcCounter),mViewportPcViewer);
	m3dViewer->setPointCloudRenderingProperties (PCL_VISUALIZER_POINT_SIZE, _pointSize, "Cloud"+to_string(mPcCounter),mViewportPcViewer);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::clearPcViewer() {
	m3dViewer->removeAllPointClouds();
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::updateStereoImages(const Mat & _left, const Mat & _right) {
	mLeftImage = _left; 
	mRightImage = _right;
	hconcat(mLeftImage, mRightImage, mPairStereo);

	imshow(mName + "_StereoViewer", mPairStereo);
}

void Gui::putBlurry(bool _left) {
	Point2i startPoint;
	if(_left)
		startPoint = Point2i(20, 30);
	else
		startPoint = Point2i(20 + mLeftImage.cols, 30);

	putText(mPairStereo, "Blurry Image", startPoint, FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255),4);
	imshow(mName + "_StereoViewer", mPairStereo);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::drawPoints(const vector<Point2f>& _points, bool _isLeft, unsigned _r, unsigned _g, unsigned _b) {
	Scalar color = Scalar(_b, _g, _r);
	Point2i offset(_isLeft?0:mLeftImage.cols, 0);

	for (Point2i point : _points) {
		circle(mPairStereo, point+offset, 3, color);
	}
	imshow(mName + "_StereoViewer", mPairStereo);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::drawBoundBoxes(const vector<Rect>& _boxes, bool _isLeft, unsigned _r, unsigned _g, unsigned _b) {
	Scalar color = Scalar(_b, _g, _r);
	int offset = _isLeft?0:mLeftImage.cols;

	for (Rect box : _boxes) {
		box.x += offset;
		rectangle(mPairStereo, box, color);
	}
	imshow(mName + "_StereoViewer", mPairStereo);
}


//---------------------------------------------------------------------------------------------------------------------
void Gui::drawPolygon(const std::vector<cv::Point2f>& _polygon, bool _isLeft, unsigned _r, unsigned _g, unsigned _b) {
	Scalar color = Scalar(_b, _g, _r);
	Point2f offset(_isLeft?0:mLeftImage.cols, 0);	

	// Draw all lines except between last and initial point.
	for (unsigned i = 0; i < _polygon.size() - 1;i++) {
		Point2f p1 = _polygon[i] + offset;
		Point2f p2 = _polygon[i+1] + offset;
		line(mPairStereo, p1, p2, color,3);
	}
	
	// Draw last line
	Point2f p1 = _polygon[0] + offset;
	Point2f p2 = _polygon[_polygon.size()-1] + offset;
	line(mPairStereo, p1, p2, color,3);

	imshow(mName + "_StereoViewer", mPairStereo);
}

//---------------------------------------------------------------------------------------------------------------------
void Gui::reprojectCloud(const pcl::PointCloud<pcl::PointXYZ>::Ptr _cloud, unsigned _r, unsigned _g, unsigned _b)
{
	vector<Point3f> points3d;
	for (const PointXYZ point : *_cloud)
		points3d.push_back(Point3f(point.x, point.y, point.z));
	vector<Point2f> reprojection1, reprojection2;
	//this should work when projecting a point cloud from the camera coordinate system, for world, we have to rotate it
	projectPoints(points3d, Mat::eye(3, 3, CV_64F), Mat::zeros(3, 1, CV_64F), mStereoCameras.camera(0).matrix(), mStereoCameras.camera(0).distCoeffs(), reprojection1);
	projectPoints(points3d, mStereoCameras.rotation(), mStereoCameras.translation(), mStereoCameras.camera(1).matrix(), mStereoCameras.camera(1).distCoeffs(), reprojection2);
	drawPoints(reprojection1, true, _r, _g, _b);
	drawPoints(reprojection2, false, _r, _g, _b);
	// Calculate convexHull
	std::vector<Point2f> convexHull1, convexHull2;
	convexHull(reprojection1, convexHull1);
	convexHull(reprojection2, convexHull2);

	drawPolygon(convexHull1, true, _r, _g, _b);
	drawPolygon(convexHull2, false, _r, _g, _b);
}

//---------------------------------------------------------------------------------------------------------------------
// Private methods
//---------------------------------------------------------------------------------------------------------------------

Gui::Gui(string _name, StereoCameras& _stereoCameras): mName(_name), m3dViewer(new PCLVisualizer (mName)), mStereoCameras(_stereoCameras) {
	m3dViewer->initCameraParameters ();
	m3dViewer->addCoordinateSystem (0.5);
	// Set up mapViewer
	m3dViewer->createViewPort (0.0, 0.0, 0.5, 1.0, mViewPortMapViewer);
	m3dViewer->setBackgroundColor (0, 0, 0, mViewPortMapViewer);
	m3dViewer->addCoordinateSystem (0.5,"XYZ_map", mViewPortMapViewer);
	PointCloud<PointXYZ>::Ptr emptyCloud(new PointCloud<PointXYZ>);
	m3dViewer->addPointCloud(emptyCloud, "map",mViewPortMapViewer);
	m3dViewer->addText ("Map Viewer", 10, 10, "MapViewer text", mViewPortMapViewer);
	// Set up pc viewer
	m3dViewer->createViewPort (0.5, 0.0, 1.0, 1.0, mViewportPcViewer);
	m3dViewer->setBackgroundColor (0.1, 0.1, 0.1, mViewportPcViewer);
	m3dViewer->addCoordinateSystem (0.5,"XYZ_pc",mViewportPcViewer);
	m3dViewer->addText ("Cloud Viewer", 10, 10, "PcViewer text", mViewportPcViewer);

	// Set up stereo viewer
	namedWindow(mName + "_StereoViewer", CV_WINDOW_AUTOSIZE);
}
//---------------------------------------------------------------------------------------------------------------------
PointCloud<PointXYZRGB>::Ptr Gui::colorizePointCloud(const PointCloud<PointXYZ>::Ptr & _cloud, int _r, int _g, int _b) {
	PointCloud<PointXYZRGB>::Ptr colorizedCloud(new PointCloud<PointXYZRGB>);
	for (PointXYZ point : *_cloud) {
		PointXYZRGB p;
		p.x = point.x;
		p.y = point.y;
		p.z = point.z;
		p.r = _r;
		p.g = _g;
		p.b = _b;
		colorizedCloud->push_back(p);
	}
	return colorizedCloud;
}

//---------------------------------------------------------------------------------------------------------------------