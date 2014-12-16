/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: Message.cpp,v 8.3 2010/12/12 15:48:25 root Exp root $
*/

#include "externs.h"
#include "Message.h"
#include "TraceThread.h"
#include "Screen/LKSurface.h"
#include "Window/WndTextEdit.h"

class WndMessage : public WndTextEdit {
public:
    WndMessage() : WndTextEdit() { }
    
protected:
    virtual bool OnLButtonUp(const POINT& Pos) {
        Message::Acknowledge(0);
        return false;
    }
};

/*

  - Single window, created in GUI thread.
     -- hidden when no messages for display
     -- shown when messages available
     -- disappear when touched
     -- disappear when return clicked
     -- disappear when timeout
     -- disappear when extern event triggered
  - Message properties
     -- have a start time (seconds)
     -- timeout (start time + delta)
  - Messages stay in a circular buffer can be reviewed
  - Optional logging of all messages to file
  - Thread locking so available from any thread

*/

Poco::Mutex  CritSec_Messages;

RECT Message::rcmsg;
WndMessage Message::WndMsg;

Message::messages_t Message::messages; // from older to newer
Message::messages_t Message::messagesHistory; // from newer to older

bool Message::hidden=false;
int Message::nvisible=0;

std::tstring Message::msgText;

// Get start time to reduce overrun errors
Poco::Timestamp startTime;

int Message::block_ref = 0;

void Message::Initialize(RECT rc) {

    block_ref = 0;
    hidden = true;
    nvisible = 0;
    rcmsg = rc; // default; message window can be full size of screen

    WndMsg.Create(&MainWindow, rc);

    // change message font for different resolutions
    // Caution, remember to set font also in Resize..
    WndMsg.SetFont(ScreenLandscape ? LK8InfoBigFont : MapWindowBoldFont);
    WndMsg.SetTextColor(LKColor(0x00,0x00,0x00));
    WndMsg.SetBkColor(LKColor(0xFF,0xFF,0xFF));
}


void Message::Destroy() {
  // destroy window
    WndMsg.Destroy();
}


void Message::Lock() {
  CritSec_Messages.lock();
}

void Message::Unlock() {
  CritSec_Messages.unlock();
}



void Message::Resize() {
  SIZE tsize;
  const size_t size = msgText.size();
  RECT rthis;
  //  RECT mRc;

  if (size==0) {
    if (!hidden) {
        WndMsg.SetVisible(false);
    }
    hidden = true;
  } else {
    
    WndMsg.SetWndText(msgText.c_str());

    LKWindowSurface Surface(WndMsg);
    const auto oldfont = Surface.SelectObject(ScreenLandscape
                                              ? LK8InfoBigFont
                                              : MapWindowBoldFont);

    Surface.GetTextSize(msgText.c_str(), size, &tsize);

    Surface.SelectObject(oldfont); // 100215

    const int linecount = max(nvisible, max(1, WndMsg.GetLineCount()));

    int width =// min((rcmsg.right-rcmsg.left)*0.8,tsize.cx);
      (int)((rcmsg.right-rcmsg.left)*0.9);
    int height = (int)min((rcmsg.bottom-rcmsg.top)*0.8,(double)tsize.cy*(linecount+1));
    int h1 = height/2;
    int h2 = height-h1;

    int midx = (rcmsg.right+rcmsg.left)/2;
    int midy = (rcmsg.bottom+rcmsg.top)/2;

    rthis.left = midx-width/2;
    rthis.right = midx+width/2;
    rthis.top = midy-h1;
    rthis.bottom = midy+h2;

    WndMsg.SetTopWnd();
    WndMsg.Move(rthis, true);
    WndMsg.SetVisible(true);
    hidden = false;
    

  }

}


void Message::BlockRender(bool doblock) {
  //Lock();
  if (doblock) {
    block_ref++;
  } else {
    block_ref--;
  }
  // TODO code: add blocked time to messages' timers so they come
  // up once unblocked.
  //Unlock();
}

bool Message::Render() {
    if (!GlobalRunning) return false;
    if (block_ref) return false;

    Lock();
    Poco::Timespan fpsTime = startTime.elapsed();

    // this has to be done quickly, since it happens in GUI thread
    // at subsecond interval
    msgText.clear();
    nvisible = 0;
    bool changed = false;
    messages_t::iterator msgIt = messages.begin();
    while (msgIt != messages.end()) {
        if (msgIt->type == 0) {
            // ignore unknown messages, remove it.
            messages.erase(msgIt++);
            changed = true;
            continue;
        }

        if (msgIt->texpiry < fpsTime && msgIt->texpiry > msgIt->tstart) {
            // this message has expired, move to history list
            messagesHistory.splice(messagesHistory.begin(), messages, msgIt++);
            changed = true;
            continue;
        }

        if (msgIt->texpiry == msgIt->tstart) {
            // new message has been added, set new expiry time.
            msgIt->texpiry = fpsTime + msgIt->tshow;
            changed = true;
        }

        if (nvisible > 0) {
            msgText += TEXT("\r\n"); // add a line separator
        }
        msgText += msgIt->text; // Append Text

        ++nvisible;
        ++msgIt; // advance to next
    }
    if(messagesHistory.size() > 20) {
        // don't save more than 20 message into history.
        messagesHistory.erase(--messagesHistory.end());
    }

    if (changed || (!hidden && messages.empty())) {
        Resize();
    }
    Unlock();
    return changed;
}

void Message::AddMessage(DWORD tshow, int type, const TCHAR* Text) {

    Lock();

    const Poco::Timespan fpsTime = startTime.elapsed();
    messages.emplace_back((Message_t){Text, type, fpsTime, fpsTime, tshow*1000});

    Unlock();
}

void Message::Repeat(int type) {
    Lock();
    if (!messagesHistory.empty()) {

        // copy most recent message from history to active message.
        messages_t::iterator It = messagesHistory.begin();
        (*It).texpiry = (*It).tstart = startTime.elapsed();

        messages.splice(messages.end(), messagesHistory, It);
    }
    Unlock();
}

bool Message::Acknowledge(int type) {
    Lock();
    bool ret = false; // Did we acknowledge?

    messages_t::iterator msgIt = messages.begin();
    while (msgIt != messages.end()) {
        if (type == 0 || msgIt->type == type) {
            messagesHistory.splice(messagesHistory.begin(), messages, msgIt++);
            ret = true;
        }
    }

    Unlock();
    //  Render(); NO! this can cause crashes
    return ret;
}

