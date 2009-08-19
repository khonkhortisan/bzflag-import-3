/* bzflag
 * Copyright (c) 1993 - 2009 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// interface headers
#include "HUDuiTypeIn.h"

// system implementation headers
#include <ctype.h>

// common implementation headers
#include "FontManager.h"
#include "bzUnicode.h"
#include "LocalFontFace.h"


//
// HUDuiTypeIn
//

HUDuiTypeIn::HUDuiTypeIn()
: HUDuiControl()
, maxLength(0)
, cursorPos(data.c_str())
{
  allowEdit = true; // allow editing by default
  obfuscate = false;
}


HUDuiTypeIn::~HUDuiTypeIn()
{
}


void HUDuiTypeIn::setObfuscation(bool on)
{
  obfuscate = on;
}


size_t HUDuiTypeIn::getMaxLength() const
{
  return maxLength;
}


std::string HUDuiTypeIn::getString() const
{
  return data;
}


void HUDuiTypeIn::setMaxLength(size_t _maxLength)
{
  maxLength = _maxLength;
  setString(data.substr(0, maxLength));
  onSetFont();
}


void HUDuiTypeIn::setString(const std::string& _string)
{
  data = _string;
  cursorPos = data.c_str();
  while (*cursorPos)
    ++cursorPos;
  onSetFont();
}


// allows composing, otherwise not
void HUDuiTypeIn::setEditing(bool _allowEdit)
{
  allowEdit = _allowEdit;
}


bool HUDuiTypeIn::decrementCursor()
{
  size_t pos = cursorPos.getCount();
  if (pos <= 1) {
    return false;
  }
  --pos;
  cursorPos = data.c_str(); // reset the cursor
  while ((cursorPos.getCount() < pos) && (*cursorPos)) {
    ++cursorPos;
  }
  return true;
}


bool HUDuiTypeIn::doKeyPress(const BzfKeyEvent& key)
{
  static unsigned int backspace = '\b';	// ^H
  static unsigned int whitespace = ' ';

  if (HUDuiControl::doKeyPress(key)) {
    return true;
  }

  if (!allowEdit) {
    return false; //or return true ??
  }

  unsigned int c = key.chr;

  if (c == 0) {
    switch (key.button) {
      case BzfKeyEvent::Left: {
        if ((key.shift & BzfKeyEvent::AltKey) != 0) {
          return false;
        }
        if ((key.shift & BzfKeyEvent::ControlKey) == 0) {
          // skip a character
          decrementCursor();
        }
        else {
          // skip a word
          while (decrementCursor() &&  iswspace(*cursorPos)) {}
          while (decrementCursor() && !iswspace(*cursorPos)) {}
          if (iswspace(*cursorPos)) {
            ++cursorPos; // move into the first character of the word
          }
        }
        return true;
      }
      case BzfKeyEvent::Right: {
        if ((key.shift & BzfKeyEvent::AltKey) != 0) {
          return false;
        }
        if ((key.shift & BzfKeyEvent::ControlKey) == 0) {
          // skip a character
          if (*cursorPos) {
            ++cursorPos;
          }
        }
        else {
          // skip a word
          while (*cursorPos &&  iswspace(*cursorPos)) { ++cursorPos; }
          while (*cursorPos && !iswspace(*cursorPos)) { ++cursorPos; }
        }
        return true;
      }
      case BzfKeyEvent::Home: {
        cursorPos = data.c_str();
        return true;
      }
      case BzfKeyEvent::End: {
        while (*cursorPos) {
          ++cursorPos;
        }
        return true;
      }
      case BzfKeyEvent::Backspace: {
        c = backspace;
        break;
      }
      case BzfKeyEvent::Delete: {
        if (*cursorPos) {
          ++cursorPos;
          c = backspace;
        } else {
          return true;
        }
        break;
      }
      default: {
        return false;
      }
    }
  }

  if (!iswprint(c) && (c != backspace)) {
    return false;
  }

  if (c == backspace) {
    size_t pos = cursorPos.getCount();
    if (pos == 1) {
      goto noRoom;
    }
    else {
      // copy up to cursor position - 1
      --pos;
      cursorPos = data.c_str();
      while (cursorPos.getCount() < pos) {
        ++cursorPos;
      }
      const char* s = data.c_str();
      const char* a = cursorPos.getBufferFromHere();
      ++cursorPos;
      const char* b = cursorPos.getBufferFromHere();

      // remove the character from the data
      const std::string tmpData = data.substr(0, (a - s)) + data.substr(b - s);
      data = tmpData;

      // new buffer, restart cursor
      cursorPos = data.c_str();
      while (cursorPos.getCount() < pos) {
	++cursorPos;
      }

      if (cb != NULL) {
        cb(this, userData);
      }
    }
  }
  else {
    if (iswspace(c)) {
      c = whitespace;
    }
    CountUTF8StringItr cusi(data.c_str());
    while (*cusi) ++cusi;
    if (cusi.getCount() >= maxLength) {
      goto noRoom;
    }
    bzUTF8Char ch(c);
    size_t pos = (cursorPos.getBufferFromHere() - data.c_str());
    // copy to the current cursor location
    std::string temp = data.substr(0, pos);
    // insert the new character
    temp += ch.str();
    // copy the rest of the string
    temp += data.substr(pos, data.length());
    data = temp;
    // new buffer, restart cursor
    pos = cursorPos.getCount();
    cursorPos = data.c_str();
    while (cursorPos.getCount() < pos) {
      ++cursorPos;
    }
    // bump the cursor
    ++cursorPos;

    if (cb != NULL) {
      cb(this, userData);
    }
  }

  onSetFont();

  return true;

noRoom:
  // ring bell?
  return true;
}


bool HUDuiTypeIn::doKeyRelease(const BzfKeyEvent& key)
{
  if (key.chr == '\t' || !iswprint(key.chr))	// ignore non-printing and tab
    return false;

  // slurp up releases
  return true;
}


void HUDuiTypeIn::doRender()
{
  if (getFontFace() < 0) return;

  // render string
  glColor3fv(hasFocus() ? textColor : dimTextColor);

  FontManager &fm = FontManager::instance();
  std::string renderStr;
  if (obfuscate) {
    CountUTF8StringItr cusi(data.c_str());
    while (*cusi) ++cusi;
    renderStr.append(cusi.getCount(), '*');
  } else {
    renderStr = data;
  }
  fm.drawString(getX(), getY(), 0, getFontFace()->getFMFace(), getFontSize(), renderStr);

  // find the position of where to draw the input cursor
  float start = fm.getStringWidth(getFontFace()->getFMFace(), getFontSize(),
    renderStr.substr(0, cursorPos.getBufferFromHere() - data.c_str()));

  if (hasFocus() && allowEdit) {
    fm.drawString(getX() + start, getY(), 0, getFontFace()->getFMFace(), getFontSize(), "_");
  }
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
