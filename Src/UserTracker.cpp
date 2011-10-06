/*
 * UserTracker.cpp
 *
 *  Created on: Aug 28, 2011
 *      Author: guojianwu
 */

#include "UserTracker.h"
#include "SkeletonStruct.h"

UserTracker* UserTracker::mInstance = NULL;
UserTracker* UserTracker::getInstance()
{
	if (mInstance == NULL) {
		mInstance = new UserTracker();
	}
	return mInstance;
}

UserTracker::UserTracker()
{
	activeUserId = 0;
	activeMotionTolerance = 2.0f;
	activeTickTotlerance = 5;
	totalDist = 0;
	ni = WuCinderNITE::getInstance();
	mSignalConnectionNewUser = ni->signalNewUser.connect( boost::bind(&UserTracker::onNewUser, this, boost::lambda::_1) );
	mSignalConnectionLostUser = ni->signalLostUser.connect( boost::bind(&UserTracker::onLostUser, this, boost::lambda::_1) );
}

UserTracker::~UserTracker()
{
	mSignalConnectionNewUser.disconnect();
	mSignalConnectionLostUser.disconnect();
}

void UserTracker::release()
{
	if (mInstance != NULL) {
		delete mInstance;
		mInstance = NULL;
	}
}

void UserTracker::onNewUser(XnUserID nId)
{
	onLostUser(nId);
	mUsers.push_back(UserInfo(nId));
}

void UserTracker::onLostUser(XnUserID nId)
{
	for(list<UserInfo>::iterator it = mUsers.begin(); it != mUsers.end();) {
		if (it->id == nId) {
			mUsers.erase(it);
			break;
		}
		it++;
	}
}

void UserTracker::update()
{
	totalDist = 0;
	float confidence = 0.5f;
	// measure distance of important joints have moved from the last position
	// and decide if the user is active or not - used for sorting, and gives us
	// the next active user, if user A stays still for too long (possible lost of user)
	for(list<UserInfo>::iterator it = mUsers.begin(); it != mUsers.end();) {
		SKELETON::SKELETON &skeleton = ni->skeletons[it->id];
		if (skeleton.isTracking) {

			ci::Vec3f &shoulderL = skeleton.joints[XN_SKEL_LEFT_SHOULDER].confidence > confidence
					? skeleton.joints[XN_SKEL_LEFT_SHOULDER].position : it->shoulderL;

			ci::Vec3f &handL = skeleton.joints[XN_SKEL_RIGHT_SHOULDER].confidence > confidence
					? skeleton.joints[XN_SKEL_RIGHT_SHOULDER].position : it->shoulderR;

			ci::Vec3f &kneeL = skeleton.joints[XN_SKEL_LEFT_HAND].confidence > confidence
					? skeleton.joints[XN_SKEL_LEFT_HAND].position : it->handL;

			ci::Vec3f &shoulderR = skeleton.joints[XN_SKEL_RIGHT_HAND].confidence > confidence
					? skeleton.joints[XN_SKEL_RIGHT_HAND].position : it->handR;

			ci::Vec3f &handR = skeleton.joints[XN_SKEL_LEFT_KNEE].confidence > confidence
					? skeleton.joints[XN_SKEL_LEFT_KNEE].position : it->kneeL;

			ci::Vec3f &kneeR = skeleton.joints[XN_SKEL_RIGHT_KNEE].confidence > confidence
					? skeleton.joints[XN_SKEL_RIGHT_KNEE].position : it->kneeR;

			totalDist = 0;
			int validJoints = 0;
			if( skeleton.joints[XN_SKEL_LEFT_SHOULDER].confidence == 1 ) {
				totalDist += shoulderL.distanceSquared(it->shoulderL);

				std::cout << totalDist << std::endl;
				++validJoints;
			}

			if( skeleton.joints[XN_SKEL_RIGHT_SHOULDER].confidence == 1 ) {
				totalDist += shoulderR.distanceSquared(it->shoulderR);
				std::cout << totalDist << std::endl;
				++validJoints;
			}

			if( skeleton.joints[XN_SKEL_LEFT_HAND].confidence == 1 ) {
				totalDist += handL.distanceSquared(it->handL);

				std::cout << totalDist << std::endl;
				++validJoints;
			}

			if( skeleton.joints[XN_SKEL_RIGHT_HAND].confidence == 1 ) {
				totalDist += handR.distanceSquared(it->handR);

				std::cout << totalDist << std::endl;
				++validJoints;
			}

			if( skeleton.joints[XN_SKEL_LEFT_KNEE].confidence == 1 ) {
				totalDist += kneeL.distanceSquared(it->kneeL);

				std::cout << totalDist << std::endl;
				++validJoints;
			}

			if( skeleton.joints[XN_SKEL_RIGHT_KNEE].confidence == 1 ) {
				totalDist += kneeR.distanceSquared(it->kneeR);

				std::cout << totalDist << std::endl;
				++validJoints;
			}


			std::cout << "TotalDist: " << totalDist << " ValidJoints: " << validJoints << std::endl;

			if (totalDist <= activeMotionTolerance) {
				if (++it->motionAtZeroDuration == activeTickTotlerance) {
					it->isActive = false;
				}
			} else {
				it->motionAtZeroDuration = 0;
				if (!it->isActive) {
					it->isActive = true;
				}
			}



			std::cout << "Total Delta:"<< totalDist << std::endl;
//			std::cout << "Total Delta:"<< totalDist << std::endl;

			it->shoulderL = shoulderL;
			it->shoulderR = shoulderR;
			it->handL = handL;
			it->handR = handR;
			it->kneeL = kneeL;
			it->kneeR = kneeR;
		} else {
			it->isActive = false;
		}
		it++;
	}
	mUsers.sort();

	if (!mUsers.empty() && mUsers.begin()->isActive) {
		if (mUsers.begin()->id != activeUserId) {
			activeUserId = mUsers.begin()->id;
			ci::app::console() << mUsers.begin()->id << endl;
		}
	} else if (activeUserId != 0) {
		ci::app::console() << "no active user" << endl;
		activeUserId = 0;
	}
}

