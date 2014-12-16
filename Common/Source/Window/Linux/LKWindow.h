/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   Window.h
 * Author: Bruno de Lacheisserie
 *
 * Created on 9 novembre 2014, 14:49
 */

#ifndef _LINUX_WINDOW_H_
#define _LINUX_WINDOW_H_

#include <assert.h>
#include <tchar.h>
#include <list>
#include "utils/stl_utils.h"
#include "utils/tstring.h"
#include "Compiler.h"
#include "Screen/FontReference.h"
#include "Screen/LKFont.h"
#include "ScreenCoordinate.h"
#include "Screen/Point.hpp"
#include "Event/Timer.hpp"

#include "Screen/LKSurface.h"

template<class _Base>
class LKWindow : public _Base,
                 public Timer
{
public:
    LKWindow() = default;
    virtual ~LKWindow() {
        StopTimer();
    }
    
    virtual void SetWndText(const TCHAR* lpszText) = 0;
    virtual const TCHAR* GetWndText() const = 0;
    
    
    void Move(const RECT& rect, bool bRepaint) {
        _Base::Move(rect);
        if(bRepaint) {
            this->Invalidate();
        }
    }
    
    void SetVisible(bool Visible) { 
        if(Visible) {
            this->Show(); 
        } else {
            this->Hide();
        }
    }
    
    void Enable(bool Enable) {
        this->SetEnabled(Enable);
    }
    
    void Close() {
        this->Destroy();
    };
    
    void SetFont(FontReference Font) {
        assert(Font);
        if(Font && Font->IsDefined()) {
            _Base::SetFont(*Font);
        }
    }

    virtual void Redraw(const RECT& Rect) { 
        this->Invalidate();
    }
    
    virtual void Redraw() {
        this->Invalidate();
    }
    
    void SetToForeground() {
        this->BringToTop();
    }
    void SetTopWnd() {
        this->BringToTop();
    }
    
    static Window* GetFocus();

    void StartTimer(unsigned uTime /*millisecond*/) {
        Schedule(uTime);
    }
    
    void StopTimer() {
        Cancel();
    }
    
    void AddChild(Window* pWnd);
    void RemoveChild(Window* pWnd);
    
    virtual void OnTimer() {}

    virtual void OnPaint(Canvas &canvas) {
        LKSurface Surface; 
        Surface.Attach(&canvas);
        OnPaint(Surface, this->GetClientRect());
        _Base::OnPaint(canvas);
    }
    
    virtual void OnPaint(Canvas &canvas, const PixelRect &dirty) {
        LKSurface Surface; 
        Surface.Attach(&canvas);
        OnPaint(Surface, dirty);
        _Base::OnPaint(canvas);
    }
    
    virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys) {
        return (!_Base::OnMouseMove(x,y,keys)) && OnMouseMove((POINT){x,y});
    }
    
    virtual bool OnMouseDown(PixelScalar x, PixelScalar y) {
            return (!_Base::OnMouseDown(x,y)) && OnLButtonDown((POINT){x,y});
    }
    
    virtual bool OnMouseUp(PixelScalar x, PixelScalar y) {
        return (!_Base::OnMouseUp(x,y)) && OnLButtonUp((POINT){x,y});
    }

    virtual bool OnMouseMove(const POINT& Pos) { return false; }
    virtual bool OnLButtonDown(const POINT& Pos) { return false; }
    virtual bool OnLButtonUp(const POINT& Pos) { return false; }
  
protected:
    virtual bool OnPaint(LKSurface& Surface, const RECT& Rect) { return false; }
    
private:
    std::tstring _szWndName;
};

#endif // _LINUX_WINDOW_H_
