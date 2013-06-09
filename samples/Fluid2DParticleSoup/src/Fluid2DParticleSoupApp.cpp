/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#include "Config.h"

#include "cinder/app/AppNative.h"
#if defined( USE_DIRECTX )
  #include "cinder/app/RendererDx.h"
  #include "cinder/dx/dx.h"
  #include "cinder/dx/DxTexture.h"
#else
  #include "cinder/gl/gl.h"
  #include "cinder/gl/Texture.h"
#endif
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"

#include "cinderfx/Fluid2D.h"
#include "ParticleSoup.h"

#if defined( USE_DIRECTX )
  #pragma comment( lib, "d3d11" )
#endif

class Fluid2DParticleSoupApp : public ci::app::AppNative {
public:
	void prepareSettings( ci::app::AppNative::Settings *settings );
	void setup();
	void keyDown( ci::app::KeyEvent event );
	void mouseDown( ci::app::MouseEvent event );	
	void mouseDrag( ci::app::MouseEvent event );
	void touchesBegan( ci::app::TouchEvent event );
	void touchesMoved( ci::app::TouchEvent event );
	void touchesEnded( ci::app::TouchEvent event );
	void update();
	void draw();

private:
	float					mVelScale;
	float					mDenScale;
	float					mRgbScale;
	ci::Vec2f				mPrevPos;
std::map<int, ci::Vec2f>	mPrevTouchPos;
	cinderfx::Fluid2D		mFluid2D;

#if defined( USE_DIRECTX )
	ci::dx::TextureRef		mTex;
	ci::dx::TextureRef		mBanner;
#else
	ci::gl::TextureRef		mTex;
	ci::params::InterfaceGl	mParams;
#endif

	ParticleSoup			mParticleSoup;
	ci::Colorf				mColor;
};

using namespace ci;
using namespace ci::app;
using namespace cinderfx;
using namespace std;

void Fluid2DParticleSoupApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 700, 700 );
   	settings->setResizable( false ); 
#if ! defined( CINDER_WINRT )
	settings->setFrameRate( 1000 );
#endif
	settings->enableMultiTouch();
}

void Fluid2DParticleSoupApp::setup()
{
	console() << "WindowSize: " << getWindowSize() << std::endl;

	mDenScale = 50;
	mRgbScale = 40;

#if defined( CINDER_WINRT )
	//mFluid2D.set( 192, 108 );
	mFluid2D.set( 128, 72 );
	mFluid2D.setDensityDissipation( 0.99f );
	mDenScale = 25;  
	mVelScale = 0.0042f*std::max( mFluid2D.resX(), mFluid2D.resY() );

	mBanner = dx::Texture::create( loadImage( loadAsset( "WinRT+Cinder.png" ) ) );
#else
	mFluid2D.set( 192, 192 );
	mFluid2D.setDensityDissipation( 0.99f );
	mDenScale = 25;  
	mVelScale = 3.0f*std::max( mFluid2D.resX(), mFluid2D.resY() );
#endif

	//mFluid2D.set( 192, 192 );
	//mFluid2D.setDensityDissipation( 0.99f );
	//mFluid2D.setRgbDissipation( 0.99f ); 
	//mVelScale = 3.0f*std::max( mFluid2D.resX(), mFluid2D.resY() );


#if defined( USE_DIRECTX )
#else
	mParams = params::InterfaceGl( "Params", Vec2i( 300, 400 ) );
	mParams.addParam( "Stam Step", mFluid2D.stamStepAddr() );
	mParams.addSeparator();
	mParams.addParam( "Velocity Input Scale", &mVelScale, "min=0 max=10000 step=1" );
	mParams.addParam( "Density Input Scale", &mDenScale, "min=0 max=1000 step=1" );
	mParams.addParam( "Rgb Input Scale", &mRgbScale, "min=0 max=1000 step=1" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Dissipation", mFluid2D.velocityDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addParam( "Density Dissipation", mFluid2D.densityDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addParam( "Rgb Dissipation", mFluid2D.rgbDissipationAddr(), "min=0.0001 max=1 step=0.0001" );     
	mParams.addSeparator();
	mParams.addParam( "Velocity Viscosity", mFluid2D.velocityViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addParam( "Density Viscosity", mFluid2D.densityViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addParam( "Rgb Viscosity", mFluid2D.rgbViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addSeparator();
	mParams.addParam( "Vorticity Confinement", mFluid2D.enableVorticityConfinementAddr() );
	mParams.addSeparator();
	std::vector<std::string> boundaries;
	boundaries.push_back( "None" ); boundaries.push_back( "Wall" ); boundaries.push_back( "Wrap" );
	mParams.addParam( "Boundary Type", boundaries, mFluid2D.boundaryTypeAddr() );
	mParams.addSeparator();
	mParams.addParam( "Enable Buoyancy", mFluid2D.enableBuoyancyAddr() );
	mParams.addParam( "Buoyancy Scale", mFluid2D.buoyancyScaleAddr(), "min=0 max=100 step=0.001" );
	mParams.addParam( "Vorticity Scale", mFluid2D.vorticityScaleAddr(), "min=0 max=1 step=0.001" );
#endif	

	//mFluid2D.setRgbDissipation( 0.9930f );
	//mFluid2D.enableDensity();
	//mFluid2D.enableRgb();
	mFluid2D.enableVorticityConfinement();
	mFluid2D.setBoundaryType( Fluid2D::BOUNDARY_TYPE_WRAP );
	mFluid2D.initSimData();

	mParticleSoup.setup( &mFluid2D );

	mColor = Colorf( 0.98f, 0.7f, 0.4f );
}

void Fluid2DParticleSoupApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_r:
		mFluid2D.initSimData();
		break;
	}
}

void Fluid2DParticleSoupApp::mouseDown( MouseEvent event )
{
	mPrevPos = event.getPos();
}

void Fluid2DParticleSoupApp::mouseDrag( MouseEvent event )
{
	float x = (event.getX()/(float)getWindowWidth())*mFluid2D.resX();
	float y = (event.getY()/(float)getWindowHeight())*mFluid2D.resY();	
	
	if( event.isLeftDown() ) {
		Vec2f dv = event.getPos() - mPrevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatRgb( x, y, mRgbScale*mColor );
		if( mFluid2D.isBuoyancyEnabled() ) {
			mFluid2D.splatDensity( x, y, mDenScale );
		}
	}
	
	mPrevPos = event.getPos();
}

void Fluid2DParticleSoupApp::touchesBegan( TouchEvent event )
{
	const auto& touches = event.getTouches();
	for( const auto& touch : touches ) {
		mPrevTouchPos[touch.getId()] = Vec2f( touch.getPos() );
	}
}

void Fluid2DParticleSoupApp::touchesMoved( TouchEvent event )
{
	const auto& touches = event.getTouches();
	for( const auto& touch : touches ) {
		Vec2f prevPos = mPrevTouchPos[touch.getId()];
		Vec2f pos = touch.getPos();
		float x = (pos.x/(float)getWindowWidth())*mFluid2D.resX();
		float y = (pos.y/(float)getWindowHeight())*mFluid2D.resY();	
		Vec2f dv = pos - prevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatDensity( x, y, mDenScale );
	}

/*
	const std::vector<TouchEvent::Touch>& touches = event.getTouches();
	for( std::vector<TouchEvent::Touch>::const_iterator cit = touches.begin(); cit != touches.end(); ++cit ) {
		Vec2f prevPos = cit->getPrevPos();
		Vec2f pos = cit->getPos();
		float x = (pos.x/(float)getWindowWidth())*mFluid2D.resX();
		float y = (pos.y/(float)getWindowHeight())*mFluid2D.resY();	
		Vec2f dv = pos - prevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatRgb( x, y, mRgbScale*mColor );
		if( mFluid2D.isBuoyancyEnabled() ) {
			mFluid2D.splatDensity( x, y, mDenScale );
		}
	}
*/
}

void Fluid2DParticleSoupApp::touchesEnded( TouchEvent event )
{
}

void Fluid2DParticleSoupApp::update()
{
	mFluid2D.step();
	mParticleSoup.setColor( mColor );
	mParticleSoup.update();
}

void Fluid2DParticleSoupApp::draw()
{

#if defined( USE_DIRECTX )
	// clear out the window with black
	dx::clear( Color( 0, 0, 0 ) );
	dx::enableAdditiveBlending();

	// Uncomment to see underlining density	- not quite working yet
	/*
	float* data = const_cast<float*>( (float*)mFluid2D.rgb().data() );
	Surface32f surf( data, mFluid2D.resX(), mFluid2D.resY(), mFluid2D.resX()*sizeof(Colorf), SurfaceChannelOrder::RGB );
	if ( ! mTex ) {
		mTex = dx::Texture::create( surf );
	} else {
		mTex->update( surf );
	}
	dx::draw( mTex, getWindowBounds() );
	*/
	
	mParticleSoup.draw();

	if( mBanner ) {
		dx::disableAlphaBlending();
		Rectf bannerRect = mBanner->getBounds();
		bannerRect.scale( 0.7f );
		bannerRect = bannerRect.getCenteredFit( getWindowBounds(), false );

		dx::color( Color( 1, 1, 1 ) );
		dx::drawSolidRect( bannerRect );

		dx::color( Color( 0.7f, 0.7f, 0.7f ) );
		dx::drawStrokedRect( bannerRect );

		dx::color( Color( 1, 1, 1 ) );
		dx::draw( mBanner, bannerRect.scaledCentered( 0.96f ) );
	}

#else 
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	gl::enableAdditiveBlending();
	
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	// Uncomment to see underlining density
	/*
	float* data = const_cast<float*>( (float*)mFluid2D.rgb().data() );
	Surface32f surf( data, mFluid2D.resX(), mFluid2D.resY(), mFluid2D.resX()*sizeof(Colorf), SurfaceChannelOrder::RGB );
	if ( ! mTex ) {
		mTex = gl::Texture::create( surf );
	} else {
		mTex->update( surf );
	}
	gl::draw( mTex, getWindowBounds() );
	mTex->unbind();
	*/

	mParticleSoup.draw();
//	mParams.draw();	
#endif
}

#if defined( USE_DIRECTX )
  CINDER_APP_NATIVE( Fluid2DParticleSoupApp, RendererDx )
#else
  CINDER_APP_NATIVE( Fluid2DParticleSoupApp, RendererGl )
#endif
