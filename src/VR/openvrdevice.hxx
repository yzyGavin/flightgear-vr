// opevrdevice.hxx -- OpenVR Device Setup
//
// Written by Andrea Zanoni, started Aug 2018
//
// Copyright (C) 2018  Andrea Zanoni - <andrea.zanoni at polimi.it>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

/* Original Implementaton part of OSGOpenVRViewer:
 * 	https://github.com/ChrisDenham/osgopenvrviewer
 *      Author: Chris Denham
 *      Licence: 3-clause BSD-licence
 */

#ifndef _OSG_OPENVRDEVICE_H_
#define _OSG_OPENVRDEVICE_H_

// Include the OpenVR SDK
#include <openvr.h>

#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Version>
#include <osg/FrameBufferObject>

#if(OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))
	typedef osg::GLExtensions OSG_GLExtensions;
	typedef osg::GLExtensions OSG_Texture_Extensions;
#else
	typedef osg::FBOExtensions OSG_GLExtensions;
	typedef osg::Texture::Extensions OSG_Texture_Extensions;
#endif

class OpenVRTextureBuffer : public osg::Referenced
{
public:
	OpenVRTextureBuffer(osg::ref_ptr<osg::State> state, int width, int height, int msaaSamples);
	void destroy(osg::GraphicsContext* gc);
	GLuint getTexture() { return m_Resolve_ColorTex; }
	int textureWidth() const { return m_width; }
	int textureHeight() const { return m_height; }
	int samples() const { return m_samples; }
	void onPreRender(osg::RenderInfo& renderInfo);
	void onPostRender(osg::RenderInfo& renderInfo);

protected:
	~OpenVRTextureBuffer() {}

	friend class OpenVRMirrorTexture;
	GLuint m_Resolve_FBO; // MSAA FBO is copied to this FBO after render.
	GLuint m_Resolve_ColorTex; // color texture for above FBO.
	GLuint m_MSAA_FBO; // framebuffer for MSAA RTT
	GLuint m_MSAA_ColorTex; // color texture for MSAA RTT 
	GLuint m_MSAA_DepthTex; // depth texture for MSAA RTT
	GLint m_width; // width of texture in pixels
	GLint m_height; // height of texture in pixels
	int m_samples;  // sample width for MSAA

};

class OpenVRMirrorTexture : public osg::Referenced
{
public:
	OpenVRMirrorTexture(osg::ref_ptr<osg::State> state, GLint width, GLint height);
	void destroy(osg::GraphicsContext* gc);
	void blitTexture(osg::GraphicsContext* gc, OpenVRTextureBuffer* leftEye,  OpenVRTextureBuffer* rightEye);
protected:
	~OpenVRMirrorTexture() {}

	GLuint m_mirrorFBO;
	GLuint m_mirrorTex;
	GLint m_width;
	GLint m_height;
};

class OpenVRInitialDrawCallback : public osg::Camera::DrawCallback
{
public:
	OpenVRInitialDrawCallback(void)
	{
	}

	virtual void operator()(osg::RenderInfo& renderInfo) const;
};

class OpenVRPreDrawCallback : public osg::Camera::DrawCallback
{
public:
	OpenVRPreDrawCallback(osg::Camera* camera, OpenVRTextureBuffer* textureBuffer)
		: m_camera(camera)
		, m_textureBuffer(textureBuffer)
	{
	}

	virtual void operator()(osg::RenderInfo& renderInfo) const;
protected:
	osg::Camera* m_camera;
	OpenVRTextureBuffer* m_textureBuffer;

};

class OpenVRPostDrawCallback : public osg::Camera::DrawCallback
{
public:
	OpenVRPostDrawCallback(osg::Camera* camera, OpenVRTextureBuffer* textureBuffer)
		: m_camera(camera)
		, m_textureBuffer(textureBuffer)
	{
	}

	virtual void operator()(osg::RenderInfo& renderInfo) const;
protected:
	osg::Camera* m_camera;
	OpenVRTextureBuffer* m_textureBuffer;

};


class OpenVRDevice : public osg::Referenced
{

public:
	typedef enum Eye_
	{
		LEFT = 0,
		RIGHT = 1,
		COUNT = 2
	} Eye;
	OpenVRDevice(float nearClip, float farClip, const float worldUnitsPerMetre = 1.0f, const int samples = 0);
	void createRenderBuffers(osg::ref_ptr<osg::State> state);
	
	osg::ref_ptr<OpenVRTextureBuffer> getTextureBuffer(OpenVRDevice::Eye eye) const {return m_textureBuffer[eye];};

	void init();
	void shutdown(osg::GraphicsContext* gc);

	static bool hmdPresent();
	bool hmdInitialized() const;

	osg::Matrix projectionMatrixCenter() const;
	osg::Matrix projectionMatrixLeft() const;
	osg::Matrix projectionMatrixRight() const;

	osg::Matrix projectionOffsetMatrixLeft() const;
	osg::Matrix projectionOffsetMatrixRight() const;

	osg::Matrix viewMatrixLeft() const;
	osg::Matrix viewMatrixRight() const;

	float nearClip() const { return m_nearClip;	}
	float farClip() const { return m_farClip; }

	void resetSensorOrientation() const;
	void updatePose();

	osg::Vec3 position() const { return m_position; }
	osg::Quat orientation() const { return m_orientation;  }

	osg::Camera* createRTTCamera(OpenVRDevice::Eye eye, osg::Transform::ReferenceFrame referenceFrame, const osg::Vec4& clearColor, osg::GraphicsContext* gc = 0) const;
	void prepareRTTCameraViewport(osg::Camera* camera);
	void getRTTCameraViewportDims(uint32_t& renderWidth, uint32_t& renderHeight);

	bool submitFrame();
	void blitMirrorTexture(osg::GraphicsContext* gc);

	osg::GraphicsContext::Traits* graphicsContextTraits() const;
protected:
	~OpenVRDevice(); // Since we inherit from osg::Referenced we must make destructor protected

	void calculateEyeAdjustment();
	void calculateProjectionMatrices();

	void trySetProcessAsHighPriority() const;

	vr::IVRSystem* m_vrSystem;
	vr::IVRRenderModels* m_vrRenderModels;
	const float m_worldUnitsPerMetre;

	osg::ref_ptr<OpenVRTextureBuffer> m_textureBuffer[2];
	osg::ref_ptr<OpenVRMirrorTexture> m_mirrorTexture;

	osg::Matrixf m_leftEyeProjectionMatrix;
	osg::Matrixf m_rightEyeProjectionMatrix;
	osg::Vec3f m_leftEyeAdjust;
	osg::Vec3f m_rightEyeAdjust;

	osg::Vec3 m_position;
	osg::Quat m_orientation;

	float m_nearClip;
	float m_farClip;
	int m_samples;
private:
	std::string GetDeviceProperty(vr::TrackedDeviceProperty prop);
	OpenVRDevice(const OpenVRDevice&); // Do not allow copy
	OpenVRDevice& operator=(const OpenVRDevice&); // Do not allow assignment operator.
};


class OpenVRRealizeOperation : public osg::GraphicsOperation
{
public:
	explicit OpenVRRealizeOperation(osg::ref_ptr<OpenVRDevice> device) :
		osg::GraphicsOperation("OpenVRRealizeOperation", false), m_device(device), m_realized(false) {}
	virtual void operator () (osg::GraphicsContext* gc);
	bool realized() const { return m_realized; }
protected:
	OpenThreads::Mutex  _mutex;
	osg::observer_ptr<OpenVRDevice> m_device;
	bool m_realized;
};


class OpenVRSwapCallback : public osg::GraphicsContext::SwapCallback
{
public:
	explicit OpenVRSwapCallback(osg::ref_ptr<OpenVRDevice> device) : m_device(device), m_frameIndex(0) {}
	void swapBuffersImplementation(osg::GraphicsContext* gc);
	int frameIndex() const { return m_frameIndex; }
private:
	osg::observer_ptr<OpenVRDevice> m_device;
	int m_frameIndex;
};


#endif /* _OSG_OPENVRDEVICE_H_ */
