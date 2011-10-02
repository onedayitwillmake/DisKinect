#include "cinder/app/AppBasic.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/Color.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"

#include "UserRelay.h"
#include "Puppeteer.h"

#include <OpenGL.framework/Headers/gl.h>
#include <OpenGL.framework/Headers/glext.h>

#include "WuCinderNITE.h"
#include "UserTracker.h"
#include "SkeletonStruct.h"

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

	void keyUp(KeyEvent event);
	relay::UserRelay* userRelay;
	puppeteer::Puppeteer* puppetier;
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
//	aNi->setup("Resources/SkeletonRec.oni");
	aNi->setup("Resources/Sample-User.xml", mapMode, true, true);
//	aNi->startUpdating();
	aNi->mContext.StartGeneratingAll();

	UserTracker* aTracker = UserTracker::getInstance();

	userRelay = new relay::UserRelay( aNi, aTracker );
	puppetier = new puppeteer::Puppeteer();
}


void DisKinect::update()
{
	userRelay->update();

}

void DisKinect::draw()
{
	gl::clear(ColorA(0, 0, 0, 0), true);
	WuCinderNITE::getInstance()->renderDepthMap(getWindowBounds());
	userRelay->draw();
	userRelay->renderSkeleton();

	SKELETON::SKELETON skeleton = userRelay->getSkeleton();
	puppetier->update(skeleton);
}

void DisKinect::shutdown()
{
	console() << "quitting..." << std::endl;
	delete userRelay;
}

void DisKinect::keyUp(KeyEvent event)

{
	if (event.getChar() == KeyEvent::KEY_q) {
		quit();
	}
}

CINDER_APP_BASIC( DisKinect, RendererGl )
