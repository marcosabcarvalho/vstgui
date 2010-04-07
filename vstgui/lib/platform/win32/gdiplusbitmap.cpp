//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
//
// Version 4.0
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2010, Steinberg Media Technologies, All Rights Reserved
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

#include "gdiplusbitmap.h"

#if WINDOWS

#include "win32support.h"

namespace VSTGUI {

//-----------------------------------------------------------------------------
GdiplusBitmap::GdiplusBitmap ()
: bitmap (0)
{
	GDIPlusGlobals::enter ();
}

//-----------------------------------------------------------------------------
GdiplusBitmap::GdiplusBitmap (const CPoint& size)
: bitmap (0)
, size (size)
{
	GDIPlusGlobals::enter ();
	bitmap = new Gdiplus::Bitmap ((INT)size.x, (INT)size.y, PixelFormat32bppARGB);
}

//-----------------------------------------------------------------------------
GdiplusBitmap::~GdiplusBitmap ()
{
	if (bitmap)
		delete bitmap;
	GDIPlusGlobals::exit ();
}

//-----------------------------------------------------------------------------
bool GdiplusBitmap::load (const CResourceDescription& desc)
{
	if (bitmap == 0)
	{
		if (gCustomBitmapReaderCreator)
		{
			// TODO: gCustomBitmapReaderCreator win32 support
			#if DEBUG
			DebugPrint ("TODO: gCustomBitmapReaderCreator win32 support\n");
			#endif
		}
		ResourceStream* resourceStream = new ResourceStream;
		if (resourceStream->open (desc, "PNG"))
			bitmap = Gdiplus::Bitmap::FromStream (resourceStream, TRUE);
		resourceStream->Release ();
		if (!bitmap)
			bitmap = Gdiplus::Bitmap::FromResource (GetInstance (), desc.type == CResourceDescription::kIntegerType ? (WCHAR*)MAKEINTRESOURCE(desc.u.id) : (WCHAR*)desc.u.name);
		if (bitmap)
		{
			size.x = (CCoord)bitmap->GetWidth ();
			size.y = (CCoord)bitmap->GetHeight ();
			return true;
		}
	}
	return false;
}

} // namespace

#endif // WINDOWS