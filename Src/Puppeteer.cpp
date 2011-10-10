#include "cinder/app/App.h"
#include "cinder/CinderMath.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/PolyLine.h"
#include "cinder/Quaternion.h"
#include "Puppeteer.h"
#include "XnTypes.h"

#include "Constants.h"

#define CHECK_DELTA(val1, val2) math<float>::abs(val1 - val2) >= .25f ? val1 : val2

using namespace cinder;

namespace puppeteer {

Puppeteer::Puppeteer() {
	lastUpdateTime = 0.0f;
	updateInterval = 0.5f;
	arduinoUnit = 10.0f;
	if (Constants::Debug::USE_ARDUINO) {
		arduino = new ArduinoCommandInterface();
		arduino->setup("tty.usbmodemfd131", false);
	}
}

Puppeteer::~Puppeteer() {
	if (Constants::Debug::USE_ARDUINO) {
		delete arduino;
	}
}

void Puppeteer::update(SKELETON::SKELETON& skeleton)
{
	if ( skeleton.joints[XN_SKEL_LEFT_SHOULDER].confidence == 0 || skeleton.joints[XN_SKEL_RIGHT_SHOULDER].confidence == 0 ) {
		return;
	}
	// ----------------------------legs

	Vec3f &lHip = skeleton.joints[XN_SKEL_LEFT_HIP].position;
	Vec3f &rHip = skeleton.joints[XN_SKEL_RIGHT_HIP].position;
	Vec3f &lKnee = skeleton.joints[XN_SKEL_LEFT_KNEE].position;
	Vec3f &rKnee = skeleton.joints[XN_SKEL_RIGHT_KNEE].position;
	float legLenL = lKnee.distance(lHip);
	float legLegR = rKnee.distance(rHip);
	float legPosL = arduinoUnit * math<float>::clamp(1.0f - (lHip.y - lKnee.y + legLenL * .25f) / (legLenL * 1.25f), 0.0f, 1.0f);
	float legPosR = arduinoUnit * math<float>::clamp(1.0f - (rHip.y - rKnee.y + legLegR * .25f) / (legLegR * 1.25f), 0.0f, 1.0f);

	legPosL = CHECK_DELTA(legPosL, lastLegPosL);
	legPosR = CHECK_DELTA(legPosR, lastLegPosR);

	lastLegPosL = legPosL;
	lastLegPosR = legLegR;

	// ----------------------------hands
	shoulderL = skeleton.joints[XN_SKEL_LEFT_SHOULDER].position;
	shoulderR = skeleton.joints[XN_SKEL_RIGHT_SHOULDER].position;

	// calculate length for both left and right arms based on skeleton size
	armLenL = shoulderL.distance(skeleton.joints[XN_SKEL_LEFT_ELBOW].position)
			+ skeleton.joints[XN_SKEL_LEFT_ELBOW].position.distance(skeleton.joints[XN_SKEL_LEFT_HAND].position);
	armLenR = shoulderR.distance(skeleton.joints[XN_SKEL_RIGHT_ELBOW].position)
			+ skeleton.joints[XN_SKEL_RIGHT_ELBOW].position.distance(skeleton.joints[XN_SKEL_RIGHT_HAND].position);

	// get the 3 axis aligned to the body
	axisHoriz = (skeleton.joints[XN_SKEL_LEFT_SHOULDER].position - skeleton.joints[XN_SKEL_RIGHT_SHOULDER].position).normalized();
	axisVert = (skeleton.joints[XN_SKEL_NECK].position - skeleton.joints[XN_SKEL_TORSO].position).normalized();
	normal = axisHoriz.cross(axisVert).normalized();

	Vec3f v1 = Vec3f(0, 0, -1).normalized();
	Vec3f v2 = normal;
	// align rectangular region to z-axis
	Quatf q = Quatf( v1.cross(v2).normalized(), -acos(v1.dot(v2)) );
	Matrix33<float> m1 = Matrix33<float>::createRotation(q.getAxis(), q.getAngle());
	// normal aligned with z-axis but rectangular region is rotated around the z-axis, we need to undo this rotation
	Vec3f p = m1.transformVec(axisVert);
	float theta = atan2(p.y, p.x);
	Matrix33<float> m2 = Matrix33<float>::createRotation(Vec3f::zAxis(), -theta + M_PI / 2.0f);
	normalizationMatrix = m2 * m1;

	handL = normalizationMatrix.transformVec(skeleton.joints[XN_SKEL_LEFT_HAND].position - shoulderL);
	handR = normalizationMatrix.transformVec(skeleton.joints[XN_SKEL_RIGHT_HAND].position - shoulderR);

	// ----------------------------send to arduino
	if (cinder::app::App::get()->getElapsedSeconds() - lastUpdateTime >= updateInterval) {
		lastUpdateTime = cinder::app::App::get()->getElapsedSeconds();
		Vec3f handPosL = Vec3f(
			arduinoUnit * math<float>::clamp((handL.x / armLenL) * -1.0f, 0.0f, 1.0f),
			arduinoUnit * math<float>::clamp((handL.y + armLenL) / (armLenL * 2.0f), 0.0f, 1.0f),
			arduinoUnit * math<float>::clamp((handL.z / armLenL) * -1.0f, 0.0f, 1.0f)
		);
		Vec3f handPosR = Vec3f(
			arduinoUnit * math<float>::clamp((handR.x / armLenR), 0.0f, 1.0f),
			arduinoUnit * math<float>::clamp((handR.y + armLenR) / (armLenR * 2.0f), 0.0f, 1.0f),
			arduinoUnit * math<float>::clamp((handR.z / armLenR) * -1.0f, 0.0f, 1.0f)
		);

		handL.x = CHECK_DELTA(handL.x, lastHandPosL.x);
		handL.y = CHECK_DELTA(handL.y, lastHandPosL.y);
		handL.z = CHECK_DELTA(handL.z, lastHandPosL.z);
		handR.x = CHECK_DELTA(handR.x, lastHandPosR.x);
		handR.y = CHECK_DELTA(handR.y, lastHandPosR.y);
		handR.z = CHECK_DELTA(handR.z, lastHandPosR.z);

		std::ostringstream message;
		message << round(handPosL.x) << "," << round(handPosL.y) << "," << round(handPosL.z) << ","
				<< round(handPosR.x) << "," << round(handPosR.y) << "," << round(handPosR.z) << ","
				<< round(legPosL) << ","
				<< round(legPosR) << "|";

		std::cout << message.str() << std::endl;
		if (Constants::Debug::USE_ARDUINO) {
			arduino->sendMessage(message.str());
		}

		lastHandPosL.x = handPosL.x;
		lastHandPosL.y = handPosL.y;
		lastHandPosL.z = handPosL.z;
		lastHandPosR.x = handPosR.x;
		lastHandPosR.y = handPosR.y;
		lastHandPosR.z = handPosR.z;
	}


}

void Puppeteer::draw()
{
	// ----------------------------debug

	if (Constants::Debug::DRAW_PUPPETEER_BOUNDS) {
		Vec3f origin = Vec3f::zero();
		PolyLine<Vec3f> boundsL;
		boundsL.push_back(origin - axisVert * armLenL);
		boundsL.push_back(origin - axisVert * armLenL + axisHoriz * armLenL);
		boundsL.push_back(origin + axisVert * armLenL + axisHoriz * armLenL);
		boundsL.push_back(origin + axisVert * armLenL);
		boundsL.push_back(origin - axisVert * armLenL);
		boundsL.push_back(origin - axisVert * armLenL + normal * armLenL); //
		boundsL.push_back(origin - axisVert * armLenL + normal * armLenL + axisHoriz * armLenL);
		boundsL.push_back(origin + axisVert * armLenL + normal * armLenL + axisHoriz * armLenL);
		boundsL.push_back(origin + axisVert * armLenL + normal * armLenL);
		boundsL.push_back(origin - axisVert * armLenL + normal * armLenL);

		PolyLine<Vec3f> boundsR;
		boundsR.push_back(origin - axisVert * armLenR);
		boundsR.push_back(origin - axisVert * armLenR - axisHoriz * armLenR);
		boundsR.push_back(origin + axisVert * armLenR - axisHoriz * armLenR);
		boundsR.push_back(origin + axisVert * armLenR);
		boundsR.push_back(origin - axisVert * armLenR);
		boundsR.push_back(origin - axisVert * armLenR + normal * armLenR); //
		boundsR.push_back(origin - axisVert * armLenR + normal * armLenR - axisHoriz * armLenR);
		boundsR.push_back(origin + axisVert * armLenR + normal * armLenR - axisHoriz * armLenR);
		boundsR.push_back(origin + axisVert * armLenR + normal * armLenR);
		boundsR.push_back(origin - axisVert * armLenR + normal * armLenR);

		gl::pushMatrices();

		MayaCamUI* mayaCam = Constants::mayaCam();
		gl::setMatrices(mayaCam->getCamera());

		float scale = .22f;
		// original bounding box
		gl::color(Color(0.5f, 0.5f, 0.5f));
		gl::pushModelView();
		gl::translate(shoulderL);
		gl::draw(boundsL);
		gl::popModelView();

		gl::pushModelView();
		gl::translate(shoulderR);
		gl::draw(boundsR);
		gl::popModelView();

		gl::color(Color(0, 0, 1));
		gl::pushModelView();
		gl::scale(scale, scale, scale);
		gl::rotate( Quatf(normalizationMatrix) );
		gl::draw(boundsL);
		gl::popModelView();

		gl::pushModelView();
		gl::scale(scale, scale, scale);
		gl::drawCube(handL, Vec3f(100.0f, 100.0f, 100.0f));
		gl::popModelView();

		gl::color(Color(0, 1, 0));
		gl::pushModelView();
		gl::scale(scale, scale, scale);
		gl::rotate( Quatf(normalizationMatrix) );
		gl::draw(boundsR);
		gl::popModelView();

		gl::pushModelView();
		gl::scale(scale, scale, scale);
		gl::drawCube(handR, Vec3f(100.0f, 100.0f, 100.0f));
		gl::popModelView();

		gl::popMatrices();

		gl::color(Color(1, 1, 1));
	}
}

}
