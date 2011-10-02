/*
 * UserStreamPlayer.h
 *
 *  Created on: Sep 26, 2011
 *      Author: mariogonzalez
 *      Abstract:
 *      	 This represents one of the states in the UserStreamStateManager
 *      	 It is responsible for playing a json skeleton recording
 */

#include "UserStreamPlayer.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include "UserStreamLive.h"
#include "UserStreamRecorder.h"
#include "UserStreamPlayer.h"


#include "WuCinderNITE.h"
#include "UserTracker.h"
#include "SkeletonStruct.h"

#include "json/reader.h"
#include "simplegui/SimpleGUI.h"

namespace relay {
	UserStreamPlayer::UserStreamPlayer() {
	}

	UserStreamPlayer::~UserStreamPlayer() {
		ni = NULL;
		tracker = NULL;

		delete _gui; _gui = NULL;
		delete _label; _label = NULL;
		delete _toggle; _toggle = NULL;

		std::cout << "UserStreamPlayer destructor!" << std::endl;
	}

	void UserStreamPlayer::enter() {
		if( _recording.size() == 0 ) {
			std::cout << "UserStreamPlayer Error: You must call 'setJsonFromString', or 'setJsonFromValue' before this state is entered" << std::endl;
			return;
		}

		ni = WuCinderNITE::getInstance();
		tracker = UserTracker::getInstance();

		_currentFrame = 0;
		_state = PLAYING;
		_shouldLoop = true;

		// GUI
		_gui = new mowa::sgui::SimpleGUI( ci::app::App::get() );
		_gui->textColor = ColorA(1,1,1,1);
		_gui->lightColor = ColorA(1, 0, 1, 1);
		_gui->darkColor = ColorA(0.05,0.05,0.05, 1);
		_gui->bgColor = ColorA(0.15, 0.15, 0.15, 1.0);
		_label = _gui->addLabel("Playing");
		_loopButton = _gui->addParam("Loop", &_shouldLoop, _shouldLoop);
		_gui->addSeparator();
		_frameSlider = _gui->addParam("Frame:", &_currentFrame, 0, _totalframes, 0);
		_gui->addSeparator();
		_toggle = _gui->addButton("Pause");
		_toggle->registerClick( this, &UserStreamPlayer::onToggleRecordingClicked );
	}

	void UserStreamPlayer::update() {

		if(_state == PLAYING && _currentFrame <= _totalframes) _currentFrame++;

		// Loop
		if( _shouldLoop && _currentFrame > _totalframes ) {
			_currentFrame = 0;
		}

		if(_frameSlider) {
			std::stringstream ss;
			ss << "Frame: " << _currentFrame << " of " << _totalframes;
			_frameSlider->name = ss.str();
		}
	}

	void UserStreamPlayer::draw() {
		_gui->draw();
	}

	void UserStreamPlayer::exit() {
		ni = NULL;
		tracker = NULL;

		delete _gui; _gui = NULL;
		delete _label; _label = NULL;
		delete _toggle; _toggle = NULL;
	}

	void UserStreamPlayer::play() {
		_state = PLAYING;
	}

	void UserStreamPlayer::pause() {
		_state = PAUSED;
	}
	void UserStreamPlayer::restart() {
		_currentFrame = 0;
		_state = PLAYING;
	}

	bool UserStreamPlayer::onToggleRecordingClicked( ci::app::MouseEvent event ) {

		if(_state == PLAYING) {
			_label->name = "PAUSED";
			_toggle->name = "PLAY";
			pause();
		} else if (_state == PAUSED ) {
			_label->name = "PLAYING";
			_toggle->name = "PAUSE";
			play();
		}

		return true;
	}

	SKELETON::SKELETON UserStreamPlayer::getSkeleton() {
		uint frameToPlay = (uint)ci::math<float>::min( _currentFrame, _totalframes-1 );

		// To avoid throwing an exception - just return an empty skeleton struct if the current frame is greater than our size
		// This can happen do to a race condition if update is called just before the file is loaded
		if( frameToPlay < _recording.size() ) {
			return _recording.at( frameToPlay )->skeleton;
		} else {
			SKELETON::SKELETON skeleton;
			return skeleton;
		}

	}


	// SAVE LOAD JSON
	void UserStreamPlayer::setJson( const std::string &aPath ) {
		Json::Value *aJsonValue = new Json::Value();
		Json::Reader reader;

		// Read the filestream
		ifstream filestream;

		// load from resources directory so we can modify it while we work
		filestream.open( ci::app::App::get()->getResourcePath(aPath).c_str(), ifstream::in);

		// Parse the json file or retrieve errors
		bool parsingSuccessful = reader.parse( filestream, *aJsonValue );
		if ( !parsingSuccessful ) {
			// report to the user the failure and their locations in the document.
			std::cout  << "Failed to parse configuration\n" << reader.getFormatedErrorMessages() << std::endl;
			return;
		}

		parseJson ( aJsonValue );
	}

	void UserStreamPlayer::setJson( Json::Value *aJsonValue ) {
		_json = aJsonValue;
		parseJson( aJsonValue );
	}

	void UserStreamPlayer::parseJson( Json::Value *aJsonValue ) {
		// Create a userframe for each json object
		Json::Value root = (*aJsonValue)["root"];

		UserStreamFrame *userFrame;
        for( Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++ ) {
        	userFrame = UserStreamFrame::fromJSON( (*itr) );
			_totalframes = userFrame->framenumber + 1;
			_recording.push_back( userFrame );
        }

        std::cout << "Playback stream created with '" << userFrame->framenumber << "' frames" << std::endl;
	}
}