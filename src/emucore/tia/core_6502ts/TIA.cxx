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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "TIA.hxx"
#include "TIATypes.hxx"
#include "M6502.hxx"

namespace TIA6502tsCore {

TIA::TIA(Console& console, Sound& sound, Settings& settings)
  : myConsole(console),
    mySound(sound),
    mySettings(settings),
    myDelayQueue(10, 20)
{
  myFrameManager.setOnFrameCompleteHandler(
    [this] () {onFrameComplete();}
  );

  myCurrentFrameBuffer  = make_ptr<uInt8[]>(160 * 320);
  myPreviousFrameBuffer = make_ptr<uInt8[]>(160 * 320);

  reset();
}

void TIA::reset()
{
  myHblankCtr = 0;
  myHctr = 0;
  myMovementInProgress = false;
  myExtendedHblank = false;
  myMovementClock = 0;
  myPriority = Priority::normal;
  myHstate = HState::blank;
  myIsFreshLine = true;
  myCollisionMask = 0;
  myLinesSinceChange = 0;
  myCollisionUpdateRequired = false;

  myLastCycle = 0;

  mySound.reset();
  myDelayQueue.reset();
  myFrameManager.reset();
}

void TIA::systemCyclesReset()
{
  uInt32 cycles = mySystem->cycles();

  myLastCycle -= cycles;
  mySound.adjustCycleCounter(-cycles);
}

void TIA::install(System& system)
{
  installDelegate(system, *this);
}

void TIA::installDelegate(System& system, Device& device)
{
  // Remember which system I'm installed in
  mySystem = &system;

  // All accesses are to the given device
  System::PageAccess access(&device, System::PA_READWRITE);

  // We're installing in a 2600 system
  for(uInt32 i = 0; i < 8192; i += (1 << System::PAGE_SHIFT))
    if((i & 0x1080) == 0x0000)
      mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);
}


// TODO: stub
bool TIA::save(Serializer& out) const
{
  return true;
}

// TODO: stub
bool TIA::load(Serializer& in)
{
  return true;
}

// TODO: stub
uInt8 TIA::peek(uInt16 address)
{
  return 0;
}

// TODO: stub
bool TIA::poke(uInt16 address, uInt8 value)
{
  return false;
}

// TODO: stub
void TIA::frameReset()
{}

// TODO: stub
bool TIA::saveDisplay(Serializer& out) const
{
  return true;
}

// TODO: stub
bool TIA::loadDisplay(Serializer& in)
{
  return true;
}

// TODO: stub
void TIA::update()
{}

uInt8* TIA::currentFrameBuffer() const
{
  return myCurrentFrameBuffer.get();
}

// TODO: stub
uInt8* TIA::previousFrameBuffer() const
{
  return myPreviousFrameBuffer.get();
}

uInt32 TIA::height() const
{
  return myFrameManager.height();
}

uInt32 TIA::ystart() const
{
  return 0;
}

// TODO: stub
void TIA::setHeight(uInt32 height)
{}

// TODO: stub
void TIA::setYStart(uInt32 ystart)
{}

// TODO: stub
void TIA::enableAutoFrame(bool enabled)
{}

// TODO: stub
void TIA::enableColorLoss(bool enabled)
{}

bool TIA::isPAL() const
{
  return myFrameManager.tvMode() == FrameManager::TvMode::pal;
}

// TODO: stub
uInt32 TIA::clocksThisLine() const
{
  return 0;
}

// TODO: stub
uInt32 TIA::scanlines() const
{
  return 0;
}

// TODO: stub
bool TIA::partialFrame() const
{
  return myFrameManager.isRendering();
}

// TODO: stub
uInt32 TIA::startScanline() const
{
  return 0;
}

// TODO: stub
bool TIA::scanlinePos(uInt16& x, uInt16& y) const
{
  return 0;
}

// TODO: stub
bool TIA::toggleBit(TIABit b, uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::toggleBits()
{
  return false;
}

// TODO: stub
bool TIA::toggleCollision(TIABit b, uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::toggleCollisions()
{
  return false;
}

// TODO: stub
bool TIA::toggleHMOVEBlank()
{
  return false;
}

// TODO: stub
bool TIA::toggleFixedColors(uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::driveUnusedPinsRandom(uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::toggleJitter(uInt8 mode)
{
  return false;
}

// TODO: stub
void TIA::setJitterRecoveryFactor(Int32 f)
{}

void TIA::cycle(uInt32 colorClocks)
{
  for (uInt32 i = 0; i < colorClocks; i++) {
    myDelayQueue.execute(
      [this] (uInt8 address, uInt8 value) {delayedWrite(address, value);}
    );

    myCollisionUpdateRequired = false;

    tickMovement();

    if (myHstate == HState::blank)
      tickHblank();
    else
      tickHframe();

    if (myCollisionUpdateRequired) updateCollision();
  }
}

void TIA::tickMovement()
{
  if (myMovementInProgress) return;

  if ((myHctr & 0x03) == 0) {
    myLinesSinceChange = 0;

    const bool apply = myHstate == HState::blank;

    bool m = false;

    // TODO: propagate movement to sprites

    myMovementInProgress = m;
    myCollisionUpdateRequired = m;

    myMovementClock++;
  }
}

void TIA::tickHblank()
{
  if (myIsFreshLine) {
    myHblankCtr = 0;
    myIsFreshLine = false;
  }

  if (++myHblankCtr >= 68) myHstate = HState::frame;
}

void TIA::tickHframe()
{
  const uInt32 y = myFrameManager.currentLine();
  const bool lineNotCached = myLinesSinceChange < 2 || y == 0;
  const uInt32 x = myHctr - 68;

  myCollisionUpdateRequired = lineNotCached;

  // TODO: playfield tick

  // TODO: render sprites

  // TODO: tick sprites

  if (myFrameManager.isRendering()) renderPixel(x, y, lineNotCached);

  if (++myHctr >= 228) nextLine();
}

void TIA::nextLine()
{
  myHctr = 0;
  myLinesSinceChange++;

  myHstate = HState::blank;
  myIsFreshLine = true;
  myExtendedHblank = false;

  myFrameManager.nextLine();
}

void TIA::updateCollision()
{
  // TODO: update collision mask with sprite masks
}

void TIA::renderPixel(uInt32 x, uInt32 y, bool lineNotCached)
{
  if (lineNotCached) {
    uInt8 color = 0;

    // TODO: determine color from sprites

    myCurrentFrameBuffer.get()[y * 160 + x] = myFrameManager.vblank() ? 0 : color;
  } else {
    myCurrentFrameBuffer.get()[y * 160 + x] = myCurrentFrameBuffer.get()[(y-1) * 160 + x];
  }
}

void TIA::onFrameComplete()
{
  mySystem->m6502().stop();
}

// TODO: stub
void TIA::delayedWrite(uInt8 address, uInt8 value)
{}

} // namespace TIA6502tsCore