#include "cinder/app/AppBasic.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/Color.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"

#include "UserRelay.h"
#include "Puppeteer.h"

#include <OpenGL.framework/Headers/gl.h>
#include <OpenGL.framework/Headers/glext.h>

#include "WuCinderNITE.h"
#include "UserTracker.h"
#include "SkeletonStruct.h"
#include "Constants.h"
#include "TimeLapseRGB.h"

using namespace ci;
using namespace app;
using namespace std;

class DisKinect : public AppBasic {
public:
	void prepareSettings( AppBasic::Settings *settings );
	void setup();
	void update();
	void draw();
	void shutdown();
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );


	void keyUp(KeyEvent event);
	relay::UserRelay* userRelay;
	puppeteer::Puppeteer* puppetier;
	TimeLapseRGB* rgbSaver;
};

void DisKinect::prepareSettings( AppBasic::Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void DisKinect::setup()
{
	XnMapOutputMode mapMode;
	mapMode.nFPS = 30;
	mapMode.nXRes = 640;
	mapMode.nYRes = 480;


	WuCinderNITE* aNi = WuCinderNITE::getInstance();
	if (Constants::Debug::USE_RECORDED_ONI) {
//		aNi->setup(getResourcePath("1.oni"));
		aNi->setup(getResourcePath("SkeletonRec.oni"));
	} else {
		aNi->setup("./Resources/Sample-User.xml", mapMode, true, true);
	}

	aNi->useCalibrationFile(getResourcePath("calibration.dat"));
//	aNi->startUpdating();
	aNi->mContext.StartGeneratingAll();



	ci::CameraPersp cam;
	cam.setPerspective(60.0f, cinder::app::App::get()->getWindowAspectRatio(), 1.0f, WuCinderNITE::getInstance()->maxDepth);
	cam.lookAt(ci::Vec3f(0, 0, -500.0f), ci::Vec3f::zero());
	Constants::mayaCam()->setCurrentCam( cam );

	UserTracker* aTracker = UserTracker::getInstance();

	userRelay = new relay::UserRelay( aNi, aTracker );
	puppetier = new puppeteer::Puppeteer();

	if( Constants::Debug::CREATE_TIMELAPSE )
		rgbSaver = new TimeLapseRGB();
}


void DisKinect::update()
{
	userRelay->update();
}

void DisKinect::draw()
{
	gl::clear( ColorA::black(), true);

	userRelay->draw();

	SKELETON::SKELETON skeleton = userRelay->getSkeleton();
	puppetier->update(skeleton);
}

void DisKinect::shutdown()
{
	console() << "quitting..." << std::endl;
	delete userRelay;
	delete puppetier;
	delete rgbSaver; rgbSaver = NULL;
}

void DisKinect::keyUp(KeyEvent event)
{
	if (event.getChar() == KeyEvent::KEY_q) {
		quit();
	}
}
void DisKinect::mouseDown( MouseEvent event )
{
	MayaCamUI* mayaCam = Constants::mayaCam();
	mayaCam->mouseDown(event.getPos());
}
void DisKinect::mouseDrag( MouseEvent event )
{
	MayaCamUI* mayaCam = Constants::mayaCam();
	mayaCam->mouseDrag(event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown());
}

CINDER_APP_BASIC( DisKinect, RendererGl )
