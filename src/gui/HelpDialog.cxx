//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "HelpDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelpDialog::HelpDialog(OSystem& osystem, DialogContainer& parent,
                       const GUI::Font& font)
  : Dialog(osystem, parent),
    myPage(1),
    myNumPages(5)
{
  const string ELLIPSIS = "\x1d";
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  _w = 46 * fontWidth + 10;
  _h = 12 * lineHeight + 20;

  // Add Previous, Next and Close buttons
  xpos = 10;  ypos = _h - buttonHeight - 10;
  myPrevButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Previous", GuiObject::kPrevCmd);
  myPrevButton->clearFlags(WIDGET_ENABLED);
  wid.push_back(myPrevButton);

  xpos += buttonWidth + 7;
  myNextButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Next", GuiObject::kNextCmd);
  wid.push_back(myNextButton);

  xpos = _w - buttonWidth - 10;
  ButtonWidget* b =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Close", GuiObject::kCloseCmd);
  wid.push_back(b);
  addOKWidget(b);  addCancelWidget(b);

  xpos = 5;  ypos = 5;
  myTitle = new StaticTextWidget(this, font, xpos, ypos, _w - 10, fontHeight,
                                 "", kTextAlignCenter);

  int lwidth = 12 * fontWidth;
  xpos += 5;  ypos += lineHeight + 4;
  for(uInt8 i = 0; i < kLINES_PER_PAGE; i++)
  {
    myKey[i] =
      new StaticTextWidget(this, font, xpos, ypos, lwidth,
                           fontHeight, "", kTextAlignLeft);
    myDesc[i] =
      new StaticTextWidget(this, font, xpos+lwidth, ypos, _w - xpos - lwidth - 5,
                           fontHeight, "", kTextAlignLeft);
    ypos += fontHeight;
  }

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::updateStrings(uInt8 page, uInt8 lines, string& title)
{
#ifdef BSPF_MAC_OSX
  #define ALT_ "Cmd"
#else
  #define ALT_ "Alt"
#endif

  int i = 0;
  auto ADD_BIND = [&](const string& k, const string& d)
  {
    myKeyStr[i] = k;  myDescStr[i] = d;  i++;
  };
  auto ADD_TEXT = [&](const string& d) { ADD_BIND("", d); };
  auto ADD_LINE = [&]() { ADD_BIND("", ""); };

  switch(page)
  {
    case 1:
      title = "Common commands";
#ifndef BSPF_MAC_OSX
      ADD_BIND("Ctrl Q",    "Quit emulation");
#else
      ADD_BIND("Cmd Q",     "Quit emulation");
#endif
      ADD_BIND("Escape",     "Exit current game");
      ADD_BIND("Tab",        "Enter 'Options' menu");
      ADD_BIND("\\",         "Toggle command menu");
      ADD_BIND(ALT_" =",     "Increase window size");
      ADD_BIND(ALT_" -",     "Decrease window size");
      ADD_BIND(ALT_" Enter", "Toggle fullscreen /");
      ADD_BIND("",           "  windowed mode");
      ADD_BIND(ALT_" ]",     "Increase volume by 2%");
      ADD_BIND(ALT_" [",     "Decrease volume by 2%");
      break;

    case 2:
      title = "Special commands";
      ADD_BIND("Ctrl g", "Grab mouse (keep in window)");
      ADD_BIND("Ctrl f", "Switch between NTSC/PAL/SECAM");
      ADD_BIND("Ctrl s", "Save game properties to a");
      ADD_BIND("",       "  new file");
      ADD_LINE();
      ADD_BIND("Ctrl 0", "Toggle controller for Mouse");
      ADD_BIND("Ctrl 1", "Toggle Stelladaptor left/right");
      break;

    case 3:
      title = "TV Filters";
      ADD_BIND(ALT_" 1", "Disable filtering");
      ADD_BIND(ALT_" 2", "Enable 'Composite' mode");
      ADD_BIND(ALT_" 3", "Enable 'S-video' mode");
      ADD_BIND(ALT_" 4", "Enable 'RGB' mode");
      ADD_BIND(ALT_" 5", "Enable 'Bad Adjust' mode");
      ADD_BIND(ALT_" 6", "Enable 'Custom' mode");
      ADD_BIND(ALT_" 7", "Adjust scanline intensity");
      ADD_BIND(ALT_" 8", "Toggle scanline interpol.");
      ADD_BIND(ALT_" 9", "Select 'Custom' adjustable");
      ADD_BIND(ALT_" 0", "Modify 'Custom' adjustable");
      break;

    case 4:
      title = "Developer commands";
      ADD_BIND("`",         "Enter/exit debugger");
      ADD_LINE();
      ADD_BIND(ALT_" PgUp", "Increase Display.YStart");
      ADD_BIND(ALT_" PgDn", "Decrease Display.YStart");
      ADD_BIND("Ctrl PgUp", "Increase Display.Height");
      ADD_BIND("Ctrl PgDn", "Decrease Display.Height");
      ADD_LINE();
      ADD_BIND("Alt L", "Toggle frame stats");
      ADD_BIND("Alt ,", "Toggle 'Debug Colors' mode");
      ADD_BIND("Alt r", "Toggle continuous rewind");
      break;

    case 5:
      title = "All other commands";
      ADD_LINE();
      ADD_BIND("Remapped Eve", "nts");
      ADD_TEXT("Most other commands can be");
      ADD_TEXT("remapped. Please consult the");
      ADD_TEXT("'Input Settings" + ELLIPSIS + "' menu for");
      ADD_TEXT("more information.");
      break;
  }

  while(i < lines)
    ADD_LINE();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::displayInfo()
{
  string titleStr;
  updateStrings(myPage, kLINES_PER_PAGE, titleStr);

  myTitle->setLabel(titleStr);
  for(uInt8 i = 0; i < kLINES_PER_PAGE; i++)
  {
    myKey[i]->setLabel(myKeyStr[i]);
    myDesc[i]->setLabel(myDescStr[i]);
  }

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::handleCommand(CommandSender* sender, int cmd,
                               int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kNextCmd:
      myPage++;
      if(myPage >= myNumPages)
        myNextButton->clearFlags(WIDGET_ENABLED);
      if(myPage >= 2)
        myPrevButton->setFlags(WIDGET_ENABLED);

      displayInfo();
      break;

    case GuiObject::kPrevCmd:
      myPage--;
      if(myPage <= myNumPages)
        myNextButton->setFlags(WIDGET_ENABLED);
      if(myPage <= 1)
        myPrevButton->clearFlags(WIDGET_ENABLED);

      displayInfo();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
