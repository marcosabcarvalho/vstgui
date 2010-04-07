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

#include "cframe.h"
#include "coffscreencontext.h"
#include "ctooltipsupport.h"
#include "animation/animator.h"
#include <assert.h>

namespace VSTGUI {

#if MAC_CARBON && MAC_COCOA
	static bool createNSViewMode = false;
#endif

const char* kMsgNewFocusView = "kMsgNewFocusView";
const char* kMsgOldFocusView = "kMsgOldFocusView";

#define DEBUG_MOUSE_VIEWS	0//DEBUG

//-----------------------------------------------------------------------------
// CFrame Implementation
//-----------------------------------------------------------------------------
/*! @class CFrame
It creates a platform dependend view object. 

On Mac OS X it is a HIView or NSView.\n 
On Windows it's a WS_CHILD Window.

*/
//-----------------------------------------------------------------------------
/**
 * @param inSize size of frame
 * @param inSystemWindow parent platform window
 * @param inEditor editor
 */
CFrame::CFrame (const CRect &inSize, void* inSystemWindow, VSTGUIEditorInterface* inEditor)
: CViewContainer (inSize, 0, 0)
, pEditor (inEditor)
, pMouseObserver (0)
, pKeyboardHook (0)
, pViewAddedRemovedObserver (0)
, pTooltips (0)
, pAnimator (0)
, pModalView (0)
, pFocusView (0)
, pActiveFocusView (0)
, bActive (true)
{
	bIsAttached = true;
	
	pParentFrame = this;

	initFrame (inSystemWindow);

}

//-----------------------------------------------------------------------------
CFrame::~CFrame ()
{
	if (pTooltips)
		pTooltips->forget ();
	if (pAnimator)
		pAnimator->forget ();

	clearMouseViews (CPoint (0, 0), 0, false);

	if (pModalView)
		removeView (pModalView, false);

	setCursor (kCursorDefault);

	pParentFrame = 0;
	removeAll ();

	if (platformFrame)
		platformFrame->forget ();
}

#if MAC_COCOA && MAC_CARBON
//-----------------------------------------------------------------------------
void CFrame::setCocoaMode (bool state)
{
	createNSViewMode = state;
}
bool CFrame::getCocoaMode ()
{
	return createNSViewMode;
}
#endif

//-----------------------------------------------------------------------------
void CFrame::close ()
{
	clearMouseViews (CPoint (0, 0), 0, false);

	if (pModalView)
		removeView (pModalView, false);
	setCursor (kCursorDefault);
	pParentFrame = 0;
	removeAll ();
	if (platformFrame)
	{
		platformFrame->forget ();
		platformFrame = 0;
	}
	forget ();
}

//-----------------------------------------------------------------------------
bool CFrame::initFrame (void* systemWin)
{
	if (!systemWin)
		return false;

	platformFrame = IPlatformFrame::createPlatformFrame (this, size, systemWin);
	if (!platformFrame)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
void CFrame::enableTooltips (bool state)
{
	if (state)
	{
		if (pTooltips == 0)
			pTooltips = new CTooltipSupport (this);
	}
	else if (pTooltips)
	{
		pTooltips->forget ();
		pTooltips = 0;
	}
}

#if VSTGUI_ENABLE_DEPRECATED_METHODS
//-----------------------------------------------------------------------------
CDrawContext* CFrame::createDrawContext ()
{
	CDrawContext* pContext = 0;
	
	return pContext;
}
#endif

//-----------------------------------------------------------------------------
void CFrame::draw (CDrawContext* pContext)
{
	return CFrame::drawRect (pContext, size);
}

//-----------------------------------------------------------------------------
void CFrame::drawRect (CDrawContext* pContext, const CRect& updateRect)
{
	if (updateRect.getWidth () <= 0 || updateRect.getHeight () <= 0 || pContext == 0)
		return;

	if (pContext)
		pContext->remember ();

	CRect oldClip;
	pContext->getClipRect (oldClip);
	CRect newClip (updateRect);
	newClip.bound (oldClip);
	pContext->setClipRect (newClip);

	// draw the background and the children
	CViewContainer::drawRect (pContext, updateRect);

	pContext->setClipRect (oldClip);

	pContext->forget ();
}

//-----------------------------------------------------------------------------
void CFrame::clearMouseViews (const CPoint& where, const long& buttons, bool callMouseExit)
{
	IMouseObserver* mouseObserver = getMouseObserver ();
	CPoint lp;
	std::list<CView*>::reverse_iterator it = pMouseViews.rbegin ();
	while (it != pMouseViews.rend ())
	{
		if (callMouseExit)
		{
			lp = where;
			(*it)->frameToLocal (lp);
			(*it)->onMouseExited (lp, buttons);
		#if DEBUG_MOUSE_VIEWS
			DebugPrint ("mouseExited : %p\n", (*it));
		#endif
		}
		if (mouseObserver)
			mouseObserver->onMouseExited ((*it), this);
		(*it)->forget ();
		it++;
	}
	pMouseViews.clear ();
}

//-----------------------------------------------------------------------------
void CFrame::removeFromMouseViews (CView* view)
{
	IMouseObserver* mouseObserver = getMouseObserver ();
	bool found = false;
	std::list<CView*>::iterator it = pMouseViews.begin ();
	while (it != pMouseViews.end ())
	{
		if (found || (*it) == view)
		{
			if (pTooltips)
				pTooltips->onMouseExited ((*it));
			if (mouseObserver)
				mouseObserver->onMouseExited ((*it), this);
			(*it)->forget ();
			pMouseViews.erase (it++);
			found = true;
		}
		else
			it++;
	}
}

//-----------------------------------------------------------------------------
void CFrame::checkMouseViews (const CPoint& where, const long& buttons)
{
	if (mouseDownView)
		return;
	CPoint lp;
	CView* mouseView = getViewAt (where, true);
	if (mouseView == 0)
		mouseView = getContainerAt (where, true);
	CView* currentMouseView = pMouseViews.size () > 0 ? pMouseViews.back () : 0;
	if (currentMouseView == mouseView)
		return; // no change

	if (pTooltips)
	{
		if (currentMouseView)
			pTooltips->onMouseExited (currentMouseView);
		if (mouseView && mouseView != this)
			pTooltips->onMouseEntered (mouseView);
	}

	if (mouseView == 0 || mouseView == this)
	{
		clearMouseViews (where, buttons);
		return;
	}
	IMouseObserver* mouseObserver = getMouseObserver ();
	CViewContainer* vc = currentMouseView ? dynamic_cast<CViewContainer*> (currentMouseView) : 0;
	// if the currentMouseView is not a view container, we know that the new mouseView won't be a child of it and that all other
	// views in the list are viewcontainers
	if (vc == 0 && currentMouseView)
	{
		lp = where;
		currentMouseView->frameToLocal (lp);
		currentMouseView->onMouseExited (lp, buttons);
		if (mouseObserver)
			mouseObserver->onMouseExited (currentMouseView, this);
	#if DEBUG_MOUSE_VIEWS
		DebugPrint ("mouseExited : %p\n", currentMouseView);
	#endif
		currentMouseView->forget ();
		pMouseViews.remove (currentMouseView);
	}
	std::list<CView*>::reverse_iterator it = pMouseViews.rbegin ();
	while (it != pMouseViews.rend ())
	{
		vc = dynamic_cast<CViewContainer*> ((*it));
		if (vc == mouseView)
			return;
		if (vc->isChild (mouseView, true) == false)
		{
			lp = where;
			vc->frameToLocal (lp);
			vc->onMouseExited (lp, buttons);
			if (mouseObserver)
				mouseObserver->onMouseExited (vc, this);
		#if DEBUG_MOUSE_VIEWS
			DebugPrint ("mouseExited : %p\n", vc);
		#endif
			vc->forget ();
			pMouseViews.erase (--it.base ());
		}
		else
			break;
	}
	vc = pMouseViews.size () > 0 ? dynamic_cast<CViewContainer*> (pMouseViews.back ()) : 0;
	if (vc)
	{
		std::list<CView*>::iterator it2 = pMouseViews.end ();
		it2--;
		while ((vc = dynamic_cast<CViewContainer*> (mouseView->getParentView ())) != *it2)
		{
			pMouseViews.insert (it2, vc);
			vc->remember ();
			mouseView = vc;
		}
		pMouseViews.push_back (mouseView);
		mouseView->remember ();
		it2++;
		while (it2 != pMouseViews.end ())
		{
			lp = where;
			(*it2)->frameToLocal (lp);
			(*it2)->onMouseEntered (lp, buttons);
			if (mouseObserver)
				mouseObserver->onMouseEntered ((*it2), this);
		#if DEBUG_MOUSE_VIEWS
			DebugPrint ("mouseEntered : %p\n", (*it2));
		#endif
			it2++;
		}
	}
	else
	{
		// must be pMouseViews.size () == 0
		assert (pMouseViews.size () == 0);
		pMouseViews.push_back (mouseView);
		mouseView->remember ();
		while ((vc = dynamic_cast<CViewContainer*> (mouseView->getParentView ())) != this)
		{
			pMouseViews.push_front (vc);
			vc->remember ();
			mouseView = vc;
		}
		std::list<CView*>::iterator it2 = pMouseViews.begin ();
		while (it2 != pMouseViews.end ())
		{
			lp = where;
			(*it2)->frameToLocal (lp);
			(*it2)->onMouseEntered (lp, buttons);
			if (mouseObserver)
				mouseObserver->onMouseEntered ((*it2), this);
		#if DEBUG_MOUSE_VIEWS
			DebugPrint ("mouseEntered : %p\n", (*it2));
		#endif
			it2++;
		}
	}
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::onMouseDown (CPoint &where, const long& buttons)
{
	// reset views
	mouseDownView = 0;
	if (pFocusView && pFocusView->isTypeOf ("CTextEdit"))
		setFocusView (0);

	if (pTooltips)
		pTooltips->onMouseDown (where);

	if (getMouseObserver ())
		getMouseObserver ()->onMouseDown (this, where);

	if (pModalView)
	{
		CBaseObjectGuard rg (pModalView);

		if (pModalView->hitTest (where, buttons))
		{
			CMouseEventResult result = pModalView->onMouseDown (where, buttons);
			if (result == kMouseEventHandled)
			{
				mouseDownView = pModalView;
				return kMouseEventHandled;
			}
		}
	}
	else
		return CViewContainer::onMouseDown (where, buttons);
	return kMouseEventNotHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::onMouseUp (CPoint &where, const long& buttons)
{
	CMouseEventResult result = CViewContainer::onMouseUp (where, buttons);
	long modifiers = buttons & (kShift | kControl | kAlt | kApple);
	checkMouseViews (where, modifiers);
	return result;
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::onMouseMoved (CPoint &where, const long& buttons)
{
	if (pTooltips)
		pTooltips->onMouseMoved (where);
	if (getMouseObserver ())
		getMouseObserver ()->onMouseMoved (this, where);

	checkMouseViews (where, buttons);

	CMouseEventResult result = kMouseEventNotHandled;
	if (pModalView)
	{
		CBaseObjectGuard rg (pModalView);
		result = pModalView->onMouseMoved (where, buttons);
	}
	else
	{
		CPoint p (where);
		result = CViewContainer::onMouseMoved (p, buttons);
		if (result == kMouseEventNotHandled)
		{
			long buttons2 = (buttons & (kShift | kControl | kAlt | kApple));
			std::list<CView*>::reverse_iterator it = pMouseViews.rbegin ();
			while (it != pMouseViews.rend ())
			{
				p = where;
				(*it)->frameToLocal (p);
				result = (*it)->onMouseMoved (p, buttons2);
				if (result == kMouseEventHandled)
					break;
				it++;
			}
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::onMouseExited (CPoint &where, const long& buttons)
{ // this should only get called from the platform implementation

	if (mouseDownView == 0)
	{
		clearMouseViews (where, buttons);
	}

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
long CFrame::onKeyDown (VstKeyCode& keyCode)
{
	long result = -1;

	if (getKeyboardHook ())
		result = getKeyboardHook ()->onKeyDown (keyCode, this);

	if (result == -1 && pFocusView)
	{
		CBaseObjectGuard og (pFocusView);
		result = pFocusView->onKeyDown (keyCode);
	}

	if (result == -1 && pModalView)
	{
		CBaseObjectGuard og (pModalView);
		result = pModalView->onKeyDown (keyCode);
	}

	if (result == -1 && keyCode.virt == VKEY_TAB)
		result = advanceNextFocusView (pFocusView, (keyCode.modifier & MODIFIER_SHIFT) ? true : false) ? 1 : -1;

	return result;
}

//-----------------------------------------------------------------------------
long CFrame::onKeyUp (VstKeyCode& keyCode)
{
	long result = -1;

	if (getKeyboardHook ())
		result = getKeyboardHook ()->onKeyUp (keyCode, this);

	if (result == -1 && pFocusView)
		result = pFocusView->onKeyUp (keyCode);

	if (result == -1 && pModalView)
		result = pModalView->onKeyUp (keyCode);

	return result;
}

//------------------------------------------------------------------------
bool CFrame::onWheel (const CPoint &where, const CMouseWheelAxis &axis, const float &distance, const long &buttons)
{
	bool result = false;

	if (mouseDownView == 0)
	{
		CView* view = pModalView ? pModalView : getViewAt (where);
		if (view)
		{
			result = view->onWheel (where, axis, distance, buttons);
			checkMouseViews (where, buttons);
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
bool CFrame::onWheel (const CPoint &where, const float &distance, const long &buttons)
{
	return onWheel (where, kMouseWheelAxisY, distance, buttons);
}

//-----------------------------------------------------------------------------
long CFrame::doDrag (CDropSource* source, const CPoint& offset, CBitmap* dragBitmap)
{
	if (platformFrame)
		return platformFrame->doDrag (source, offset, dragBitmap);
	return 0;
}

//-----------------------------------------------------------------------------
void CFrame::idle ()
{
	invalidateDirtyViews ();
}

//-----------------------------------------------------------------------------
void CFrame::doIdleStuff ()
{
	if (pEditor)
		pEditor->doIdleStuff ();
}

//-----------------------------------------------------------------------------
Animation::Animator* CFrame::getAnimator ()
{
	if (pAnimator == 0)
		pAnimator = new Animation::Animator;
	return pAnimator;
}

//-----------------------------------------------------------------------------
/**
 * @return tick count in milliseconds
 */
unsigned long CFrame::getTicks () const
{
	if (platformFrame)
		return platformFrame->getTicks ();
	return -1;
}

//-----------------------------------------------------------------------------
long CFrame::getKnobMode () const
{
	if (pEditor)
		return pEditor->getKnobMode ();
	return kCircularMode;
}

//-----------------------------------------------------------------------------
/**
 * repositions the frame
 * @param x x coordinate
 * @param y y coordinate
 * @return true on success
 */
bool CFrame::setPosition (CCoord x, CCoord y)
{
	if (platformFrame)
	{
		CRect rect (size);
		size.offset (x - size.left, y - size.top);
		return platformFrame->setSize (rect);
	}
	return false;
}

//-----------------------------------------------------------------------------
/**
 * get global position of frame
 * @param x x coordinate
 * @param y y coordinate
 * @return true on success
 */
bool CFrame::getPosition (CCoord &x, CCoord &y) const
{
	if (platformFrame)
	{
		CPoint p;
		if (platformFrame->getGlobalPosition (p))
		{
			x = p.x;
			y = p.y;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void CFrame::setViewSize (CRect& rect, bool invalid)
{
	CViewContainer::setViewSize (rect, invalid);
}

//-----------------------------------------------------------------------------
/**
 * set size of frame (and the platform representation)
 * @param width new width
 * @param height new height
 * @return true on success
 */
bool CFrame::setSize (CCoord width, CCoord height)
{
	if ((width == size.width ()) && (height == size.height ()))
		return false;

	CRect newSize (size);
	newSize.setWidth (width);
	newSize.setHeight (height);

	if (platformFrame)
	{
		if (platformFrame->setSize (newSize))
		{
			CViewContainer::setViewSize (newSize);
			return true;
		}
		return false;
	}
	CViewContainer::setViewSize (newSize);
	return true;
}

//-----------------------------------------------------------------------------
/**
 * get size relative to parent
 * @param pRect size
 * @return true on success
 */
bool CFrame::getSize (CRect* pRect) const
{
	if (platformFrame && pRect)
		return platformFrame->getSize (*pRect);
	return false;
}

//-----------------------------------------------------------------------------
bool CFrame::getSize (CRect& outSize) const
{
	return getSize (&outSize);
}

//-----------------------------------------------------------------------------
/**
 * @param pView the view which should be set to modal.
 * @return true if view could be set as the modal view. false if there is a already a modal view or the view to be set as modal is already attached.
 */
bool CFrame::setModalView (CView* pView)
{
	// If there is a modal view or the view 
	if ((pView && pModalView) || (pView && pView->isAttached ()))
		return false;

	if (pModalView)
		removeView (pModalView, false);
	
	pModalView = pView;
	if (pModalView)
		return addView (pModalView);

	return true;
}

//-----------------------------------------------------------------------------
void CFrame::beginEdit (long index)
{
	if (pEditor)
		pEditor->beginEdit (index);
}

//-----------------------------------------------------------------------------
void CFrame::endEdit (long index)
{
	if (pEditor)
		pEditor->endEdit (index);
}

//-----------------------------------------------------------------------------
/**
 * @param where location of mouse
 * @return true on success
 */
bool CFrame::getCurrentMouseLocation (CPoint &where) const
{
	if (platformFrame)
		return platformFrame->getCurrentMousePosition (where);
	return false;
}

//-----------------------------------------------------------------------------
/**
 * @return mouse and modifier state
 */
long CFrame::getCurrentMouseButtons () const
{
	long buttons = 0;

	if (platformFrame)
		platformFrame->getCurrentMouseButtons (buttons);

	return buttons;
}

//-----------------------------------------------------------------------------
/**
 * @param type cursor type see #CCursorType
 */
void CFrame::setCursor (CCursorType type)
{
	if (platformFrame)
		platformFrame->setMouseCursor (type);
}

//-----------------------------------------------------------------------------
/**
 * @param pView view which was removed
 */
void CFrame::onViewRemoved (CView* pView)
{
	removeFromMouseViews (pView);

	if (pActiveFocusView == pView)
		pActiveFocusView = 0;
	if (pFocusView == pView)
		setFocusView (0);
	if (pView->isTypeOf ("CViewContainer"))
	{
		CViewContainer* container = (CViewContainer*)pView;
		if (container->isChild (pFocusView, true))
			setFocusView (0);
	}
	if (getViewAddedRemovedObserver ())
		getViewAddedRemovedObserver ()->onViewRemoved (this, pView);
}

//-----------------------------------------------------------------------------
/**
 * @param pView view which was added
 */
void CFrame::onViewAdded (CView* pView)
{
	if (getViewAddedRemovedObserver ())
		getViewAddedRemovedObserver ()->onViewAdded (this, pView);
}

//-----------------------------------------------------------------------------
/**
 * @param pView new focus view
 */
void CFrame::setFocusView (CView *pView)
{
	static bool recursion = false;
	if (pView == pFocusView || (recursion && pFocusView != 0))
		return;

	if (!bActive)
	{
		pActiveFocusView = pView;
		return;
	}

	recursion = true;

	CView *pOldFocusView = pFocusView;
	pFocusView = pView;
	if (pFocusView && pFocusView->wantsFocus ())
	{
		pFocusView->invalid ();

		CView* receiver = pFocusView->getParentView ();
		while (receiver != this && receiver != 0)
		{
			receiver->notify (pFocusView, kMsgNewFocusView);
			receiver = receiver->getParentView ();
		}
		notify (pFocusView, kMsgNewFocusView);
	}

	if (pOldFocusView)
	{
		if (pOldFocusView->wantsFocus ())
		{
			pOldFocusView->invalid ();

			CView* receiver = pOldFocusView->getParentView ();
			while (receiver != this && receiver != 0)
			{
				receiver->notify (pOldFocusView, kMsgOldFocusView);
				receiver = receiver->getParentView ();
			}
			notify (pOldFocusView, kMsgOldFocusView);
		}
		pOldFocusView->looseFocus ();
	}
	if (pFocusView && pFocusView->wantsFocus ())
		pFocusView->takeFocus ();
	recursion = false;
}

//-----------------------------------------------------------------------------
bool CFrame::advanceNextFocusView (CView* oldFocus, bool reverse)
{
	if (pModalView)
	{
		if (pModalView->isTypeOf("CViewContainer"))
		{
			return ((CViewContainer*)pModalView)->advanceNextFocusView (oldFocus, reverse);
		}
		else if (oldFocus != pModalView)
		{
			setFocusView (pModalView);
			return true;
		}
		return false; // currently not supported, but should be done sometime
	}
	if (oldFocus == 0)
	{
		if (pFocusView == 0)
			return CViewContainer::advanceNextFocusView (0, reverse);
		oldFocus = pFocusView;
	}
	if (isChild (oldFocus))
	{
		if (CViewContainer::advanceNextFocusView (oldFocus, reverse))
			return true;
		else
		{
			setFocusView (0);
			return false;
		}
	}
	CView* parentView = oldFocus->getParentView ();
	if (parentView && parentView->isTypeOf ("CViewContainer"))
	{
		CView* tempOldFocus = oldFocus;
		CViewContainer* vc = (CViewContainer*)parentView;
		while (vc)
		{
			if (vc->advanceNextFocusView (tempOldFocus, reverse))
				return true;
			else
			{
				tempOldFocus = vc;
				if (vc->getParentView () && vc->getParentView ()->isTypeOf ("CViewContainer"))
					vc = (CViewContainer*)vc->getParentView ();
				else
					vc = 0;
			}
		}
	}
	return CViewContainer::advanceNextFocusView (oldFocus, reverse);
}

//-----------------------------------------------------------------------------
bool CFrame::removeView (CView* pView, const bool &withForget)
{
	if (pModalView == pView)
		pModalView = 0;
	return CViewContainer::removeView (pView, withForget);
}

//-----------------------------------------------------------------------------
bool CFrame::removeAll (const bool &withForget)
{
	pModalView = 0;
	pFocusView = 0;
	pActiveFocusView = 0;
	clearMouseViews (CPoint (0, 0), 0, false);
	return CViewContainer::removeAll (withForget);
}

//-----------------------------------------------------------------------------
void CFrame::onActivate (bool state)
{
	if (bActive != state)
	{
		if (state)
		{
			bActive = true;
			if (pActiveFocusView)
			{
				setFocusView (pActiveFocusView);
				pActiveFocusView = 0;
			}
			else
				advanceNextFocusView (0, false);
		}
		else
		{
			pActiveFocusView = getFocusView ();
			setFocusView (0);
			bActive = false;
		}
	}
}

//-----------------------------------------------------------------------------
bool CFrame::focusDrawingEnabled () const
{
	long attrSize;
	if (getAttributeSize ('vfde', attrSize))
		return true;
	return false;
}

//-----------------------------------------------------------------------------
CColor CFrame::getFocusColor () const
{
	CColor focusColor (kRedCColor);
	long outSize;
	getAttribute ('vfco', sizeof (CColor), &focusColor, outSize);
	return focusColor;
}

//-----------------------------------------------------------------------------
CCoord CFrame::getFocusWidth () const
{
	CCoord focusWidth = 2;
	long outSize;
	getAttribute ('vfwi', sizeof (CCoord), &focusWidth, outSize);
	return focusWidth;
}

//-----------------------------------------------------------------------------
void CFrame::setFocusDrawingEnabled (bool state)
{
	if (state)
		setAttribute ('vfde', sizeof(bool), &state);
	else
		removeAttribute ('vfde');
}

//-----------------------------------------------------------------------------
void CFrame::setFocusColor (const CColor& color)
{
	setAttribute ('vfco', sizeof (CColor), &color);
}

//-----------------------------------------------------------------------------
void CFrame::setFocusWidth (CCoord width)
{
	setAttribute ('vfwi', sizeof (CCoord), &width);
}

//-----------------------------------------------------------------------------
/**
 * @param src rect which to scroll
 * @param distance point of distance
 */
void CFrame::scrollRect (const CRect& src, const CPoint& distance)
{
	CRect rect (src);
	rect.offset (size.left, size.top);

	if (platformFrame)
	{
		if (platformFrame->scrollRect (src, distance))
			return;
	}
	invalidRect (src);
}

//-----------------------------------------------------------------------------
void CFrame::invalidate (const CRect &rect)
{
	CRect rectView;
	FOREACHSUBVIEW
	if (pV)
	{
		pV->getViewSize (rectView);
		if (rect.rectOverlap (rectView))
			pV->setDirty (true);
	}
	ENDFOREACHSUBVIEW
}

//-----------------------------------------------------------------------------
void CFrame::invalidRect (CRect rect)
{
	if (!bVisible)
		return;
	if (platformFrame)
		platformFrame->invalidRect (rect);
}

#if DEBUG
//-----------------------------------------------------------------------------
void CFrame::dumpHierarchy ()
{
	dumpInfo ();
	DebugPrint ("\n");
	CViewContainer::dumpHierarchy ();
}
#endif

//-----------------------------------------------------------------------------
bool CFrame::platformDrawRect (CDrawContext* context, const CRect& rect)
{
	drawRect (context, rect);
	return true;
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::platformOnMouseDown (CPoint& where, const long& buttons)
{
	CBaseObjectGuard bog (this);
	return onMouseDown (where, buttons);
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::platformOnMouseMoved (CPoint& where, const long& buttons)
{
	CBaseObjectGuard bog (this);
	return onMouseMoved (where, buttons);
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::platformOnMouseUp (CPoint& where, const long& buttons)
{
	CBaseObjectGuard bog (this);
	return onMouseUp (where, buttons);
}

//-----------------------------------------------------------------------------
CMouseEventResult CFrame::platformOnMouseExited (CPoint& where, const long& buttons)
{
	CBaseObjectGuard bog (this);
	return onMouseExited (where, buttons);
}

//-----------------------------------------------------------------------------
bool CFrame::platformOnMouseWheel (const CPoint &where, const CMouseWheelAxis &axis, const float &distance, const long &buttons)
{
	CBaseObjectGuard bog (this);
	return onWheel (where, axis, distance, buttons);
}

//-----------------------------------------------------------------------------
bool CFrame::platformOnDrop (CDragContainer* drag, const CPoint& where)
{
	CBaseObjectGuard bog (this);
	return onDrop (drag, where);
}

//-----------------------------------------------------------------------------
void CFrame::platformOnDragEnter (CDragContainer* drag, const CPoint& where)
{
	CBaseObjectGuard bog (this);
	return onDragEnter (drag, where);
}

//-----------------------------------------------------------------------------
void CFrame::platformOnDragLeave (CDragContainer* drag, const CPoint& where)
{
	CBaseObjectGuard bog (this);
	return onDragLeave (drag, where);
}

//-----------------------------------------------------------------------------
void CFrame::platformOnDragMove (CDragContainer* drag, const CPoint& where)
{
	CBaseObjectGuard bog (this);
	return onDragMove (drag, where);
}

//-----------------------------------------------------------------------------
bool CFrame::platformOnKeyDown (VstKeyCode& keyCode)
{
	CBaseObjectGuard bog (this);
	return onKeyDown (keyCode) == 1;
}

//-----------------------------------------------------------------------------
bool CFrame::platformOnKeyUp (VstKeyCode& keyCode)
{
	CBaseObjectGuard bog (this);
	return onKeyUp (keyCode) == 1;
}

//-----------------------------------------------------------------------------
void CFrame::platformOnActivate (bool state)
{
	if (pParentFrame)
		onActivate (state);
}

} // namespace
