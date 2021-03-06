	/*
 * UserStreamRecorder.cpp
 *
 *  Created on: Sep 21, 2011
 *      Author: mariogonzalez
 *      Abstract:
 *      	 This represents one of the states in the UserStreamStateManager
 *      	 It is able to record a user stream from the Kinect and outputing it as a JSON string
 *      	 It relies on a UserStreamLive instance for kinect data
 */

#pragma once
#ifndef UserStreamRecorder_H_
#define UserStreamRecorder_H_

#include <vector>
#include <iostream>
#include "IUserStream.h"
#include "UserStreamLive.h"
#include "UserStreamFrame.h"
#include "cinder/app/Event.h"
#include "cinder/app/MouseEvent.h"


// Forward declerations
namespace SKELETON { struct SKELETON; struct SKELETON_JOINT; }
namespace mowa { namespace sgui { class LabelControl; class ButtonControl; class SimpleGUI; }}
namespace Json { class Value; }



namespace relay {

class UserStreamRecorder : public IUserStream  {
	public:
		enum RecorderState { NOT_RECORDERING, RECORDING };

		UserStreamRecorder();
		virtual ~UserStreamRecorder();

		void enter();
		void update();
		void draw();
		void exit();
		bool wantsToExit() { return false; };

		Json::Value getRecordAsJSONValue();
		SKELETON::SKELETON getSkeleton();

		RecorderState getState() { return _state; };

		// recording
		void startRecording();	// Starts recording Kinect data
		void recordState();		// Records a single frame of user Kinect data via UserStreamFrame
		void stopRecording();	// Stops recording Kinect data
		void saveToDisk( std::string jsonString );	// Save to disk
		uint32_t getFrameNumber() { return _framenumber; };

		// Callbacks
		bool onToggleRecordingClicked( ci::app::MouseEvent event );
		bool onSaveClicked( ci::app::MouseEvent event );

	private:
		void setState( RecorderState aState );	// Sets the current state, modifies _label in place

		UserStreamLive *_livestream;		// When queried for skeleton data we just send whatever our livestream has

		// GUI
		mowa::sgui::LabelControl* _label;	// Label that displays crrent state
		mowa::sgui::ButtonControl* _toggle;	// Toggle button
		mowa::sgui::SimpleGUI* _gui;		// SimpleGUI instance

		RecorderState _state;	// Current state - either recording or idle
		uint32_t _framenumber;	// Current frame number of recording, set to zero on start


		std::vector< UserStreamFrame_ptr > _recording;	// Stores frames during recording
	};
}

#endif /* UserStreamRecorder_H_ */
