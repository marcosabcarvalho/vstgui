//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
//
// Version 4.1
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2011, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A  PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __copenglview__
#define __copenglview__

#include "cview.h"
#include "platform/iplatformopenglview.h"

#if VSTGUI_OPENGL_SUPPORT
namespace VSTGUI {

/*
TODO: Documentation

	To setup OpenGL for a normal 2D matrix use this in drawOpenGL(..):

	CRect r (getViewSize ());
	glViewport (0, 0, r.getWidth (), r.getHeight ());
	gluOrtho2D (r.left, r.right, r.bottom, r.top);
	glTranslated (r.left, r.top, 0);

*/
//-----------------------------------------------------------------------------
class COpenGLView : public CView, public IOpenGLView
{
public:
	COpenGLView (const CRect& size);
	~COpenGLView ();

	// IOpenGLView	
	virtual void drawOpenGL (const CRect& updateRect) = 0; ///< will be called when the view was marked invalid or the view was resized

	// CView
	virtual void setViewSize (const CRect& rect, bool invalid = true);
	virtual void parentSizeChanged ();
	virtual bool removed (CView* parent);
	virtual bool attached (CView* parent);
	virtual void invalidRect (const CRect& rect);

	CLASS_METHODS_NOCOPY (COpenGLView, CView)
protected:
	virtual PixelFormat* getPixelFormat () { return 0; }	///< subclasses should return a pixelformat here If they don't want to use the default one

	void updatePlatformOpenGLViewSize ();
	
	IPlatformOpenGLView* platformOpenGLView;
};

} // namespace

#endif // VSTGUI_OPENGL_SUPPORT
#endif // __copenglview__
