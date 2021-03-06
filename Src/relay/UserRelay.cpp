/*
 * UserRelay.cpp
 *
 *  Created on: Sep 21, 2011
 *      Author: mariogonzalez
 *      Abstract:
 *      	This class contains a UserStreamStateMachine which can has the following or more states (classes):
 *      		* UserStreamLive
 *      		* UserStreamPlayback
 *      		* UserStreamRecorder
 *      	It determines which state the UserStreamStateMachine should be in,
 *      	and asks the UserStreamStateMachine for skeleton information each frame
 */

#include "UserRelay.h"
#include "cinder/app/App.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"

#include "WuCinderNITE.h"
#include "UserTracker.h"
#include "SkeletonStruct.h"

#include "UserStreamLive.h"
#include "UserStreamRecorder.h"
#include "UserStreamPlayer.h"
#include "UserStreamRepeater.h"

#include "simplegui/SimpleGUI.h"
#include "Constants.h"
using namespace Constants::Debug;

namespace relay {
	UserRelay::UserRelay( WuCinderNITE* t_ni, UserTracker* t_tracker ) {
		this->ni = t_ni;
		this->tracker = t_tracker;

		setupGesturemap();

		// Create the FSM and set the initial state
		this->fsm = new relay::UserStreamStateManager();

		// Test the live stream
		relay::UserStreamLive* live = new relay::UserStreamLive();
		this->fsm->setInitialState( live );

		// Test the recorder
//		relay::UserStreamRecorder* recorder = new relay::UserStreamRecorder();
//		this->fsm->setInitialState( recorder );

		// Test using the player
//		relay::UserStreamPlayer* player = new relay::UserStreamPlayer();
//		player->setJson( ci::app::App::get()->getResourcePath("jsontest.json") );			// Note the UserStreamPlayer requires the JSON to be set before 'enter' - set via string path or Json::Value
//		this->fsm->setInitialState( player );

		// Test repeater (records in memory then plays it back)
//		UserStreamRepeater* repeater = new relay::UserStreamRepeater();
//		this->fsm->setInitialState( repeater);

		ci::app::App::get()->registerFileDrop( this, &UserRelay::fileDrop );
		setupDebug();
	}

	UserRelay::~UserRelay() {
		tracker->release();
		ni->shutdown();

		delete fsm;

		std::cout << "UserRelay destructor!" << std::endl;
	}

	void UserRelay::setupGesturemap() {
		using namespace Constants::relay::player;
		std::map<std::string, int>::iterator it = weightedGestures()->begin();
		for(; it != weightedGestures()->end(); ++it ) {

			std::pair<std::string, int> aWeightPair = (*it);
			for(int i = 0; i < aWeightPair.second; ++i) {
				gestures.push_back( aWeightPair.first );
//				std::cout << gestures.back() << std::endl;
			}
		}

	}

	void UserRelay::setupDebug() {
		// GUI
		if( USE_GUI ) {
			_debugGUI = new mowa::sgui::SimpleGUI( ci::app::App::get() );
			_debugGUI->textColor = ColorA(1,1,1,1);
			_debugGUI->lightColor = ColorA(1, 0, 1, 1);
			_debugGUI->darkColor = ColorA(0.05,0.05,0.05, 1);
			_debugGUI->bgColor = ColorA(0.15, 0.15, 0.15, 1.0);
			_debugGUI->addColumn();
			_debugGUI->addColumn();
			_debugGUI->addColumn();
			_debugGUI->addLabel( "SetState" );

			_debugGUI->addButton("Live")->registerClick( this, &UserRelay::setStateLive );
			_debugGUI->addButton("Recording")->registerClick( this, &UserRelay::setStateRecorder );
			_debugGUI->addButton("Playback")->registerClick( this, &UserRelay::setStatePlayback );
			_debugGUI->addButton("Repeater")->registerClick( this, &UserRelay::setStateRepeater );
			_debugGUI->addParam("ActivationZone", &(tracker->activationZone.z), 1.0f, 4.0f, tracker->activationZone.z);
		}
	}

	void UserRelay::update() {
		fsm->update();

		if( Constants::Debug::USE_IDLE_TIMER ) {
			relay::UserStreamLive* liveInstance = dynamic_cast<relay::UserStreamLive*>( fsm->getCurrentState() );

			if( liveInstance ) {
				if ( liveInstance->wantsToExit() ) {
					ci::app::MouseEvent event;
					setStatePlayback( event );
				}

			} else if ( tracker->activeUserId != 0 || fsm->getCurrentState()->wantsToExit() ) {
				ci::app::MouseEvent event;
				setStateLive( event );
			}
		}
	}

	void UserRelay::draw() {
		renderDepthMap();
		renderSkeleton();
		renderGUI();
		fsm->draw();
	}

	SKELETON::SKELETON UserRelay::getSkeleton() {
		return fsm->getSkeleton();
	}


	///// Debug drawing
	void UserRelay::renderGUI() {
		if( !USE_GUI ) return;
		_debugGUI->draw();
		gl::disableDepthRead();
		gl::disableDepthWrite();
		gl::enableAlphaBlending();
	}

	void UserRelay::renderDepthMap() {
		if( !DRAW_DEPTHMAP ) return;

		int windowWidth = ci::app::App::get()->getWindowWidth();
		int windowHeight = ci::app::App::get()->getWindowHeight();
		float imageSize = ci::app::App::get()->getWindowWidth() * 0.25;

		ci::gl::disableDepthRead();
		ni->renderDepthMap( ci::Area(0, windowHeight - imageSize, imageSize, windowHeight ) );
		ni->renderColor( ci::Area( imageSize, windowHeight - imageSize, imageSize * 2 , windowHeight ) );
	}

	void UserRelay::renderSkeleton() {
		if( !DRAW_SKELETON ) return;

		SKELETON::SKELETON aSkeleton = getSkeleton();

		ci::gl::pushMatrices();
		ci::gl::setMatrices( Constants::mayaCam()->getCamera() );
			ni->renderSkeleton( aSkeleton , tracker->activeUserId );
		ci::gl::popMatrices();
	}

	std::string UserRelay::getRandomGesture() {
		int index = ci::Rand::randInt( gestures.size() );
		std::string path = ci::app::App::get()->getResourcePath( gestures.at( index ) );
		return path;
	}

	// Debug state switching
	bool UserRelay::setStateLive( ci::app::MouseEvent event ) {
		this->fsm->changeState( new relay::UserStreamLive() ); return true;
	};
	bool UserRelay::setStateRecorder( ci::app::MouseEvent event ) {
		this->fsm->changeState( new relay::UserStreamRecorder() ); return true;
	};
	bool UserRelay::setStatePlayback( ci::app::MouseEvent event ) {
		UserStreamPlayer* player = new relay::UserStreamPlayer();
		player->setJson( getRandomGesture() );			// Note the UserStreamPlayer requires the JSON to be set before 'enter' - set via string path or Json::Value
		this->fsm->changeState( player );
		return true;
	};
	bool UserRelay::setStateRepeater( ci::app::MouseEvent event ) { this->fsm->changeState( new relay::UserStreamRepeater() ); return true; };

	bool UserRelay::fileDrop( ci::app::FileDropEvent event ) {
			if(event.getNumFiles() != 1) return false;
			UserStreamPlayer* player = new relay::UserStreamPlayer();
			std::string fileRef = event.getFile(0);
			std::cout << fileRef << std::endl;
			player->setJson( fileRef );
			this->fsm->changeState( player );

			return true;
		}
}
