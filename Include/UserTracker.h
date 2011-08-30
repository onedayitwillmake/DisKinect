/*
 * UserTracker.h
 *
 *  Created on: Aug 28, 2011
 *      Author: guojianwu
 */

#ifndef USERTRACKER_H_
#define USERTRACKER_H_

#pragma once

#include "cinder/Vector.h"
#include <stddef.h>
#include <XnTypes.h>
#include <boost/signals2.hpp>
#include <boost/lambda/lambda.hpp>
#include <list>
#include "XnCppWrapper.h"
#include "WuCinderNITE.h"

class UserTracker {
public:
	static UserTracker* getInstance();
	virtual ~UserTracker();
	void release();

	XnUserID	activeUserId;

private:
	struct UserInfo {
		UserInfo(XnUserID nId):id(nId),isActive(false),motionAtZeroDuration(0){};
		XnUserID	id;
		bool		isActive;

		unsigned int	motionAtZeroDuration; // ticks that the user has not moved
		ci::Vec3f	shoulderL, handL, kneeL;
		ci::Vec3f	shoulderR, handR, kneeR;

		bool operator<(UserInfo &other) {
			return isActive ? false : other.isActive;
		};
		bool operator>(UserInfo &other) {
			return isActive ? true : false;
		};
	};

	UserTracker();

	static UserTracker* mInstance;

	void onNewUser(XnUserID nId);
	void onLostUser(XnUserID nId);
	void onUpdate();

	WuCinderNITE*		ni;
	list<UserInfo>		mUsers;
	boost::signals2::connection	mSignalConnectionNewUser;
	boost::signals2::connection	mSignalConnectionLostUser;
	boost::signals2::connection mSignalConnectionUpdate;


};

#endif /* USERTRACKER_H_ */
