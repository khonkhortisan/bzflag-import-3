/* bzflag
 * Copyright (c) 1993 - 2007 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// BZFlag common header
#include "common.h"

// Interface header
#include "FontManager.h"

// System headers
#include <math.h>
#include <string>
#include "FTGLTextureFont.h"

// Global implementation headers
#include "bzfgl.h"
#include "bzfio.h"
#include "AnsiCodes.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLGState.h"
#include "TimeKeeper.h"
#include "TextUtils.h"
#include "OSFile.h"


/** initialize the singleton */
template <>
FontManager* Singleton<FontManager>::_instance = (FontManager*)0;

/** initialize underline to black */
GLfloat FontManager::underlineColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};


/**
 * default constructor, protected because of singleton
 */
FontManager::FontManager() : Singleton<FontManager>(),
			     opacity(1.0f),
			     dimFactor(0.2f),
			     darkness(1.0f)
{
  faceNames.clear();
  fontFaces.clear();
  BZDB.addCallback(std::string("underlineColor"), underlineCallback, NULL);
  BZDB.touch("underlineColor");
  OpenGLGState::registerContextInitializer(freeContext, initContext,
					   (void*)this);
}


/**
 * default destructor, protected because of singleton
 */
FontManager::~FontManager()
{
  clear();
  OpenGLGState::unregisterContextInitializer(freeContext, initContext,
					     (void*)this);
  return;
}


/**
 * release our context on state changes
 */
void FontManager::freeContext(void* data)
{
  ((FontManager*)data)->clear();
  return;
}


/**
 * initialize our context
 */
void FontManager::initContext(void* data)
{
  ((FontManager*)data)->rebuild();
  return;
}


/**
 * load a specified font
 */
int FontManager::load ( const char* file )
{
  if (!file) {
    return -1;
  }

  std::string lower = TextUtils::tolower(std::string(file));
  int id = getFaceID(lower);
  if (id > 0) {
    /* already loaded */
    return id;
  }

  OSFile tempFile;
  tempFile.osName(lower.c_str());

  FontFace face;
  face.name = tempFile.getFileName();
  face.path = file;
  face.sizes.clear();

  fontFaces.push_back(face);
  faceNames[face.name] = (int)fontFaces.size()-1;
  return (int)fontFaces.size()-1;
}


/**
 * load all fonts from a given directory, returns the number of fonts
 * that were loaded
 */
int FontManager::loadAll(std::string directory)
{
  if (directory.size() == 0)
    return 0;

  // save this in case we have to rebuild
  fontDirectory = directory;

  OSDir dir(directory);
  OSFile file;

  int count = 0;
  while (dir.getNextFile(file, "*.ttf", true)) {

    if (load(file.getFullOSPath().c_str()) != -1) {
      count++;
    } else {
      logDebugMessage(4,"Font Texture load failed: %s\n", file.getOSName().c_str());
    }
  } /* end while iteration over ttf files */

  return count;
}


/**
 * clear/erase a particular font size
 */
void FontManager::clear(int font, int size)
{
  // poof
  std::map<int,void*>::iterator itr = fontFaces[font].sizes.find(size);
  if ( itr != fontFaces[font].sizes.end() ) {
    delete (FTGLTextureFont*)fontFaces[font].sizes[size];
    fontFaces[font].sizes.erase(size);
  }
}


/**
 * destroy all the fonts, clear all the lists
 */
void FontManager::clear(void)
{
  faceNames.clear();

  for ( unsigned int i = 0; i < fontFaces.size(); i++ ) {
    // iterating in reverse is essential because clear() calls
    // erase, causing the map to shift elements
    std::map<int,void*>::reverse_iterator itr;
    for (itr = fontFaces[i].sizes.rbegin(); itr != fontFaces[i].sizes.rend(); itr++) {
      if ((*itr).second) {
	clear(i, (*itr).first);
      }
    }
  }

  fontFaces.clear();
  return;
}


/**
 * ask ftgl to compute their width so that the textures are loaded,
 * gives small performance boost by loading all glyphs at once
 * upfront.
 */
void FontManager::preloadSize ( int font, int size )
{
  if (font < 0 || size < 0) {
    return;
  }

  // make sure the font has been created
  FTGLTextureFont *fnt = (FTGLTextureFont*)getGLFont(font, size);
  if (!fnt) {
    return;
  }

  // preload
  std::string charset;
  charset = "abcdefghijklmnopqrstuvwxyz";
  charset += TextUtils::toupper(charset);
  charset += "1234567890";
  charset += "`;'/.,[]\\\"";
  charset += "<>?:{}+_)(*&^%$#@!)";
  charset += " \t"; 
  fnt->Advance(charset.c_str());
}


/**
 * rebuild just one size of a given font
 */
void FontManager::rebuildSize ( int font, int size )
{
  if (font < 0 || size < 0) {
    return;
  }

  clear(font, size);

  preloadSize(font, size);
}


/**
 * rebuild all the lists
 */
void FontManager::rebuild()
{
  for ( unsigned int i = 0; i < fontFaces.size(); i++ ) {
    // iterating in reverse is essential because rebuildSize() calls
    // erase, causing the map to shift elements
    std::map<int,void*>::reverse_iterator itr;
    for (itr = fontFaces[i].sizes.rbegin(); itr != fontFaces[i].sizes.rend(); itr++) {
      std::cout << "rebuilding font " << i << " with size " << (*itr).first << " hmm " << (*itr).second << std::endl;
      if ((*itr).second) {
	rebuildSize(i, (*itr).first);
      }
    }
  }
  loadAll(fontDirectory);
}


/**
 * returns a list of the loaded font names
 */
std::vector<std::string> FontManager::getFontList ( void )
{
  std::vector<std::string>	fontList;

  for ( unsigned int i = 0; i < fontFaces.size(); i++ ) {
    fontList.push_back(fontFaces[i].name);
  }

  return fontList;
}


/**
 * return an index to a requested font
 */
int FontManager::getFaceID ( const std::string font )
{
  if (font.size() <= 0)
    return -1;

  std::string name = TextUtils::tolower(font);

  if (faceNames.find(name) != faceNames.end()) {
    return faceNames.find(name)->second;
  }

  /* no luck finding the one requested, try anything */
  if (faceNames.size() > 0) {
    logDebugMessage(3,"Requested font %s not found, using first font loaded\n", font.c_str());
    return faceNames.begin()->second;
  }

  logDebugMessage(2,"No fonts loaded\n");
  return -1;
}


/**
 * returns the number of fonts loaded
 */
int FontManager::getNumFaces(void)
{
  return (int)fontFaces.size();
}


/**
 * given a font ID, return that font's name
 */
const char* FontManager::getFaceName(int faceID)
{
  if ((faceID < 0) || (faceID > getNumFaces())) {
    logDebugMessage(2,"Trying to fetch name for invalid Font Face ID %d\n", faceID);
    return NULL;
  }

  return fontFaces[faceID].name.c_str();
}


/**
 * return the ftgl representation for a given font of a given size
 */
void* FontManager::getGLFont ( int face, int size )
{
  if ( face < 0 || face >= (int)fontFaces.size() ) {
    std::cerr << "invalid font face specified" << std::endl;
    return NULL;
  }
	
  std::map<int,void*>::iterator itr = fontFaces[face].sizes.find(size);
  if ( itr != fontFaces[face].sizes.end() ) {
    return itr->second;
  }

  FTGLTextureFont* newFont = new FTGLTextureFont(fontFaces[face].path.c_str());
  newFont->FaceSize(size);
  newFont->UseDisplayList(true);

  fontFaces[face].sizes[size] = (void*)newFont;

  return fontFaces[face].sizes[size];
}


/**
 * main work-horse.  render the provided text with the specified font
 * size, optionally justifying to a particular alignment.
 */
void FontManager::drawString(float x, float y, float z, int faceID, float size,
			     const std::string &text, const float* resetColor, fontJustification align)
{
  if (text.size() <= 0)
    return;
  
  FTGLTextureFont* theFont = (FTGLTextureFont*)getGLFont(faceID ,size);
  if ((faceID < 0) || !theFont) {
    logDebugMessage(2,"Trying to draw with an invalid font face ID %d\n", faceID);
    return;
  }

  /* clamp to aligned coordinates */
  x = floorf(x);
  y = floorf(y);
  z = floorf(z);

  /*
   * Colorize text based on ANSI codes embedded in it.  Break the text
   * every time an ANSI code is encountered.
   */

  // sane defaults
  bool bright = true;
  bool pulsating = false;
  bool underline = false;
  // negatives are invalid, we use them to signal "no change"
  GLfloat color[4] = {-1.0f, -1.0f, -1.0f, opacity};
  if (resetColor != NULL) {
    color[0] = resetColor[0] * darkness;
    color[1] = resetColor[1] * darkness;
    color[2] = resetColor[2] * darkness;
  } else {
    resetColor = BrightColors[WhiteColor];
  }

  const float darkDim = dimFactor * darkness;

  // underline color changes for bright == false
  GLfloat dimUnderlineColor[4] = { underlineColor[0] * darkDim,
				   underlineColor[1] * darkDim,
				   underlineColor[2] * darkDim,
				   opacity };
  underlineColor[3] = opacity;

  /*
   * ANSI code interpretation is somewhat limited, we only accept values
   * which have been defined in AnsiCodes.h
   */
  bool doneLastSection = false;
  int startSend = 0;
  int endSend = (int)text.find("\033[", startSend);
  bool tookCareOfANSICode = false;
  float width = 0;
  // run at least once
  if (endSend == -1) {
    endSend = (int)text.size();
    doneLastSection = true;
  }

  // split string into parts based on the embedded ANSI codes, render each separately
  // there has got to be a faster way to do this
  while (endSend >= 0) {
    // pulsate the text, if desired
    if (pulsating)
      getPulseColor(color, color);
    // render text
    int len = endSend - startSend;
    if (len > 0) {
      const char* tmpText = text.c_str();
      // get substr width, we may need it a couple times
      width = getStringWidth(faceID, size, &tmpText[startSend]);
      glPushMatrix();

      if ( align == AlignCenter ) {
	glTranslatef(x - (width*0.5f), y, z);
      } else if ( align == AlignRight ) {
	glTranslatef(x - width, y, z);
      } else {
	glTranslatef(x, y, z);
      }
      glDepthMask(0);
      
      theFont->Render(&tmpText[startSend]);

      if (underline) {
	glEnable(GL_BLEND);
	if (bright && underlineColor[0] >= 0) {
	  glColor4fv(underlineColor);
	} else if (underlineColor[0] >= 0) {
	  glColor4fv(dimUnderlineColor);
	} else if (color[0] >= 0) {
	  glColor4fv(color);
	}
	// still have a translated matrix, these coordinates are
	// with respect to the string just drawn
	glBegin(GL_LINES);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(width, 0.0f);
	glEnd();
      }
      glDepthMask(BZDBCache::zbuffer);
      glPopMatrix();
      // x transform for next substr
      x += width;
    }
    if (!doneLastSection) {
      startSend = (int)text.find('m', endSend) + 1;
    }
    // we stopped sending text at an ANSI code, find out what it is
    // and do something about it
    if (endSend != (int)text.size()) {
      tookCareOfANSICode = false;
      std::string tmpText = text.substr(endSend, (text.find('m', endSend) - endSend) + 1);
      // colors
      for (int i = 0; i <= LastColor; i++) {
	if (tmpText == ColorStrings[i]) {
	  if (bright) {
	    color[0] = BrightColors[i][0] * darkness;
	    color[1] = BrightColors[i][1] * darkness;
	    color[2] = BrightColors[i][2] * darkness;
	  } else {
	    color[0] = BrightColors[i][0] * darkDim;
	    color[1] = BrightColors[i][1] * darkDim;
	    color[2] = BrightColors[i][2] * darkDim;
	  }
	  tookCareOfANSICode = true;
	  break;
	}
      }
      // didn't find a matching color
      if (!tookCareOfANSICode) {
	// settings other than color
	if (tmpText == ANSI_STR_RESET) {
	  bright = true;
	  pulsating = false;
	  underline = false;
	  color[0] = resetColor[0] * darkness;
	  color[1] = resetColor[1] * darkness;
	  color[2] = resetColor[2] * darkness;
	} else if (tmpText == ANSI_STR_RESET_FINAL) {
	  bright = false;
	  pulsating = false;
	  underline = false;
	  color[0] = resetColor[0] * darkDim;
	  color[1] = resetColor[1] * darkDim;
	  color[2] = resetColor[2] * darkDim;
	} else if (tmpText == ANSI_STR_BRIGHT) {
	  bright = true;
	} else if (tmpText == ANSI_STR_DIM) {
	  bright = false;
	} else if (tmpText == ANSI_STR_UNDERLINE) {
	  underline = true;
	} else if (tmpText == ANSI_STR_PULSATING) {
	  pulsating = true;
	} else if (tmpText == ANSI_STR_NO_UNDERLINE) {
	  underline = false;
	} else if (tmpText == ANSI_STR_NO_PULSATE) {
	  pulsating = false;
	} else {
	  logDebugMessage(2,"ANSI Code %s not supported\n", tmpText.c_str());
	}
      }
    }
    endSend = (int)text.find("\033[", startSend);
    if ((endSend == -1) && !doneLastSection) {
      endSend = (int)text.size();
      doneLastSection = true;
    }
  }

  return;
}


/**
 * convenience routine for passing in a font id instead of font name
 */
void FontManager::drawString(float x, float y, float z,
			     const std::string &face, float size,
			     const std::string &text,
			     const float* resetColor, fontJustification align)
{
  drawString(x, y, z, getFaceID(face), size, text, resetColor, align);
}


/**
 * returns the width of the given text string for the specifed font
 */
float FontManager::getStringWidth(int faceID, float size, const std::string &text, bool alreadyStripped)
{
  if (text.size() <= 0)
    return 0.0f;

  FTGLTextureFont* theFont = (FTGLTextureFont*)getGLFont(faceID, size);
  if ((faceID < 0) || !theFont) {
    logDebugMessage(2,"Trying to find length of string for an invalid font face ID %d\n", faceID);
    return 0.0f;
  }

  // don't include ansi codes in the length, but allow outside funcs to skip
  const std::string &stripped = alreadyStripped ? text : stripAnsiCodes(text);

  return theFont->Advance(stripped.c_str());
}


/**
 * convenience routine that returns the specified font width by face name
 */
float FontManager::getStringWidth(const std::string &face, float size,
				const std::string &text, bool alreadyStripped)
{
  return getStringWidth(getFaceID(face), size, text, alreadyStripped);
}


/**
 * returns the height of the given font size
 */
float FontManager::getStringHeight(int font, float size)
{
  FTGLTextureFont* theFont = (FTGLTextureFont*)getGLFont(font, size);

  if (!theFont)
    return 0;	

  return theFont->LineHeight();
}


/**
 * convenience routine that returns the specified font's height
 */
float FontManager::getStringHeight(std::string face, float size)
{
  return getStringHeight(getFaceID(face), size);
}


/**
 * return the pulse color
 */
void FontManager::getPulseColor(const GLfloat *color, GLfloat *pulseColor) const
{
  float pulseTime = (float)TimeKeeper::getCurrent().getSeconds();

  // depth is how dark it should get (1.0 is to black)
  float pulseDepth = BZDBCache::pulseDepth;
  // rate is how fast it should pulsate (smaller is faster)
  float pulseRate = BZDBCache::pulseRate;

  float pulseFactor = fmodf(pulseTime, pulseRate) - pulseRate /2.0f;
  pulseFactor = fabsf(pulseFactor) / (pulseRate/2.0f);
  pulseFactor = pulseDepth * pulseFactor + (1.0f - pulseDepth);

  pulseColor[0] = color[0] * pulseFactor;
  pulseColor[1] = color[1] * pulseFactor;
  pulseColor[2] = color[2] * pulseFactor;
}


/**
 * called during "underline"
 */
void FontManager::underlineCallback(const std::string &, void *)
{
  // set underline color
  const std::string uColor = BZDB.get("underlineColor");
  if (strcasecmp(uColor.c_str(), "text") == 0) {
    underlineColor[0] = 0.0f;
    underlineColor[1] = 0.0f;
    underlineColor[2] = 0.0f;
  } else if (strcasecmp(uColor.c_str(), "cyan") == 0) {
    underlineColor[0] = BrightColors[CyanColor][0];
    underlineColor[1] = BrightColors[CyanColor][1];
    underlineColor[2] = BrightColors[CyanColor][2];
  } else if (strcasecmp(uColor.c_str(), "grey") == 0) {
    underlineColor[0] = BrightColors[GreyColor][0];
    underlineColor[1] = BrightColors[GreyColor][1];
    underlineColor[2] = BrightColors[GreyColor][2];
  }
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
