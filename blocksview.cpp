//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2017-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of p44lrgraphics.
//
//  p44lrgraphics is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  p44lrgraphics is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with p44lrgraphics. If not, see <http://www.gnu.org/licenses/>.
//

#include "blocksview.hpp"

#if P44SCRIPT_FULL_SUPPORT // makes no sense w/o p44Script

using namespace p44;


// MARK: ===== Blocks definitions

static const int maxPixelsPerPiece = 4; // Tetrominoes

typedef struct {
  const char* name;
  int numPixels;
  PixelPoint pixelOffsets[maxPixelsPerPiece];
} BlockDef;


static const BlockDef blockDefs[Block::numBlockTypes] = {
  { "line",              4, { {-2,0},{-1,0},{0,0},{1,0} }}, // line
  { "square",            4, { {0,0},{1,0},{1,1},{0,1} }},   // square
  { "L",                 4, { {0,-1},{1,-1},{0,0},{0,1} }}, // L
  { "L_rev",             4, { {0,-1},{1,-1},{1,0},{1,1} }}, // L reversed
  { "T",                 4, { {-1,0},{0,0},{1,0},{0,1} }},  // T
  { "squiggly",          4, { {0,-1},{0,0},{1,0},{1,1} }},  // squiggly
  { "squiggly_rev",      4, { {1,-1},{1,0},{0,0},{0,1} }},  // squiggly reversed
};


// MARK: ===== Block

Block::Block(BlocksView& aBlocksView, BlockType aBlockType, PixelColor aColor) :
  mBlocksView(aBlocksView),
  mBlockType(aBlockType),
  mColor(aColor),
  mShown(false),
  mPos({0,0}),
  mPartRotation(0)
{
}


bool Block::getPixel(int aPixelIndex, PixelPoint& aPt)
{
  const BlockDef *bd = &blockDefs[mBlockType];
  if (aPixelIndex<0 || aPixelIndex>=bd->numPixels) return false;
  aPt.x = bd->pixelOffsets[aPixelIndex].x;
  aPt.y = bd->pixelOffsets[aPixelIndex].y;
  return true;
}


void Block::getExtents(int aOrientation, PixelCoord& aMinDx, PixelCoord& aMaxDx, PixelCoord& aMinDy, PixelCoord& aMaxDy)
{
  aMinDx=0;
  aMaxDx=0;
  aMinDy=0;
  aMaxDy=0;
  int idx=0;
  PixelPoint pt;
  while (getPositionedPixel(idx, { 0, 0 }, aOrientation, pt)) {
    if (pt.x<aMinDx) aMinDx = pt.x;
    else if (pt.x>aMaxDx) aMaxDx = pt.x;
    if (pt.y<aMinDy) aMinDy = pt.y;
    else if (pt.y>aMaxDy) aMaxDy = pt.y;
    idx++;
  }
}




bool Block::getPositionedPixel(int aPixelIndex, PixelPoint aCenter, int aOrientation, PixelPoint& aPt)
{
  PixelPoint pt;
  if (!getPixel(aPixelIndex, pt)) return false;
  switch (aOrientation) {
    default: aPt.x = aCenter.x+pt.x; aPt.y = aCenter.y+pt.y; break;
    case 1: aPt.x = aCenter.x+pt.y; aPt.y = aCenter.y-pt.x; break;
    case 2: aPt.x = aCenter.x-pt.x; aPt.y = aCenter.y-pt.y; break;
    case 3: aPt.x = aCenter.x-pt.y; aPt.y = aCenter.y+pt.x; break;
  }
  return true;
}


void Block::show(bool aShow)
{
  if (aShow!=mShown) {
    mShown = aShow;
    PixelPoint pt;
    int idx=0;
    while (getPositionedPixel(idx, mPos, mPartRotation, pt)) {
      mBlocksView.setPixel(aShow ? mColor : transparent, pt);
      idx++;
    }
  }
}


bool Block::position(PixelPoint aPt, int aOrientation, bool aOpenAtBottom)
{
  PixelPoint pt;
  int idx=0;
  bool wasShown = mShown;
  // unshow it
  show(false);
  // check that new position is free
  while (getPositionedPixel(idx, aPt, aOrientation, pt)) {
    if (!mBlocksView.isWithinPlayfield(pt, true, aOpenAtBottom) || (mBlocksView.isInContentSize(pt) && mBlocksView.contentColorAt(pt).a!=0)) {
      // collision or not within field
      if (wasShown) {
        // show again at previous position
        show(true);
      }
      // signal new position is not possible
      return false;
    }
    idx++;
  }
  // can be positioned this way, do it now
  mPos = aPt;
  mPartRotation = aOrientation;
  // show in new place
  show(true);
  // signal block at new position
  return true;
}


bool Block::move(PixelPoint aDistance, int aRot, bool aOpenAtBottom)
{
  int o = (mPartRotation + aRot) & 0x3;
  PixelPoint newPos = { mPos.x+aDistance.x, mPos.y+aDistance.y };
  return position(newPos, o, aOpenAtBottom);
}



// MARK: ===== BlocksView

BlocksView::BlocksView() :
  mPause(false),
  mNextCalculation(Never)
{
  setContentSize({ 10, 20 }); // assume default size
}


BlocksView::~BlocksView()
{
  clear();
}


void BlocksView::clear()
{
  // remove block runners if any is still running
  for (int bi=0; bi<2; bi++) {
    // clear runners
    BlockRunner *b = &mActiveBlocks[bi];
    b->block = nullptr; // forget it
  }
  inherited::clear();
}


void BlocksView::stopAnimations()
{
  setPause(true);
  inherited::stopAnimations();
}


bool BlocksView::isWithinPlayfield(PixelPoint aPt, bool aOpen, bool aOpenAtBottom)
{
  if (aPt.x<0 || aPt.x>=mContent.dx) return false; // may not extend sidewards
  if ((aOpenAtBottom || !aOpen) && aPt.y>=mContent.dy) return false; // may not extend above playfield
  if ((!aOpenAtBottom || !aOpen) && aPt.y<0) return false; // may not extend below playfield
  return true; // within
}


static int adjustOrientation(int aOrientation, bool aBottom)
{
  return aBottom ? (aOrientation+2) % 4 : aOrientation;
}


bool BlocksView::launchBlock(Block::BlockType aBlockType, PixelColor aColor, int aColumn, int aOrientation, bool aBottom, MLMicroSeconds aStepInterval)
{
  setPause(false);
  BlockRunner *b = &mActiveBlocks[aBottom ? 1 : 0];
  // remove block if any is already running
  if (b->block) b->block->show(false);
  // create new block
  b->block = BlockPtr(new Block(*this, aBlockType, aColor));
  b->movingUp = aBottom;
  b->stepInterval = aStepInterval;
  b->dropping = false;
  b->droppedsteps = 0;
  // position it
  int minx,maxx,miny,maxy;
  b->block->getExtents(adjustOrientation(aOrientation, aBottom), minx, maxx, miny, maxy);
  LOG(LOG_DEBUG, "launchblock: blocktype=%d, orientation=%d, extents: minx=%d, maxx=%d, miny=%d, maxy=%d", aBlockType, aOrientation, minx, maxx, miny, maxy);
  // make sure it is within horizontal bounds
  PixelPoint pt;
  pt.x = aColumn;
  if (pt.x+minx<0) pt.x = -minx;
  else if (pt.x+maxx>=mContent.dx) pt.x = mContent.dx-1-maxx;
  // position vertically with first pixel just visible
  if (!aBottom) {
    pt.y = mContent.dy-1-miny;
  }
  else {
    pt.y = -maxy;
  }
  LOG(LOG_DEBUG, "- positioning at row %d", pt.y);
  if (!b->block->position(pt, aOrientation, aBottom)) {
    // cannot launch a block any more
    b->block = nullptr;
    return false;
  }
  else {
    b->lastStep = MainLoop::now();
    return true;
  }
}


void BlocksView::showRunningBlocks(bool aShow)
{
  for (int i=0; i<2; i++) {
    BlockRunner *b = &mActiveBlocks[i];
    if (b->block) {
      if (aShow) b->block->show(aShow);
    }
  }
}


void BlocksView::removeRow(int aY, bool aBlockFromBottom)
{
  // unshow running blocks
  showRunningBlocks(false);
  for (int i=0; i<2; i++) {
    BlockRunner *b = &mActiveBlocks[i];
    if (b->block) b->block->show(false);
  }
  // move towards falling direction of block that has caused the row to fill
  PixelRect src;
  src.x = 0;
  src.dx = mContent.dx;
  src.y = aBlockFromBottom ? 0 : aY+1;
  src.dy = aBlockFromBottom ? aY : mContent.dy-aY-1;
  PixelPoint dest;
  dest.x = 0;
  dest.y = aBlockFromBottom ? 1 : aY;
  copyPixels(nullptr, true, src, dest, false);
  // re-show running blocks
  showRunningBlocks(true);
  makeDirty();
}


int BlocksView::findCompletedRow(bool aBlockFromBottom)
{
  // check from top to bottom relative to falling block
  int dir = aBlockFromBottom ? -1 : 1;
  PixelPoint pt;
  for (pt.y = aBlockFromBottom ? mContent.dy-1 : 0; (aBlockFromBottom ? pt.y>=0 : pt.y<mContent.dy); pt.y += dir) {
    // check each row
    bool hasGap = false;
    for (pt.x = 0; pt.x<mContent.dx; pt.x++) {
      if (contentColorAt(pt).a==0) {
        hasGap = true;
        break;
      }
    }
    if (!hasGap) {
      // full row found
      return pt.y;
    }
  }
  return -1; // none found
}



MLMicroSeconds BlocksView::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil, aNow);
  if (!mPause) {
    for (int i=0; i<2; i++) {
      BlockRunner *b = &mActiveBlocks[i];
      if (b->block && b->stepInterval) {
        MLMicroSeconds nxt = b->lastStep+b->stepInterval;
        if (aNow>=nxt) {
          if (b->block->move({ 0, b->movingUp ? 1 : -1 }, 0, b->movingUp)) {
            // block could move
            b->lastStep = aNow;
            if (b->dropping) b->droppedsteps++; // count dropped steps
            makeDirty();
            updateNextCall(nextCall, nxt);
          }
          else {
            // could not move, means that we've collided with floor or existing pixels
            b->block->show(true); // paint it for the last time
            b->stepInterval = Never; // prevent from moving any further
            // Note: do not disable, so event processing can still access it until the next one starts
            // send collision
            sendEvent(newBlocksEventObj(i));
            // Todo for game
            // - maybe dim block
            // - count droppedsteps toward score
            // - set down block
            // - check rows
            // - delete full rows
            // - start new block
            // - if start not possible -> game over
          }
        }
      }
    }
  }
  return nextCall;
}


void BlocksView::moveBlock(PixelPoint aDistance, int aRot, bool aLower)
{
  BlockRunner *b = &mActiveBlocks[aLower ? 1 : 0];
  if (b->block) {
    if (b->block->move(aDistance, aRot, aLower)) {
      makeDirty();
    }
  }
}


void BlocksView::dropBlock(MLMicroSeconds aDropStepInterval, bool aLower)
{
  BlockRunner *b = &mActiveBlocks[aLower ? 1 : 0];
  if (b->block) {
    b->stepInterval = aDropStepInterval;
    b->dropping = true;
    b->droppedsteps = 0;
  }
}


void BlocksView::showBlock(bool aShow, bool aLower)
{
  BlockRunner *b = &mActiveBlocks[aLower ? 1 : 0];
  if (b->block) b->block->show(aShow);
}


void BlocksView::setdownBlock(bool aLower)
{
  BlockRunner *b = &mActiveBlocks[aLower ? 1 : 0];
  b->block = nullptr;
}



void BlocksView::recolorBlock(PixelColor aColor, bool aLower)
{
  BlockRunner *b = &mActiveBlocks[aLower ? 1 : 0];
  if (!b->block) return;
  b->block->show(false);
  b->block->mColor = aColor;
  b->block->show(true);
}



// MARK: - scripting support

using namespace P44Script;

ScriptObjPtr BlocksView::newViewObj()
{
  return new BlocksViewObj(this);
}


/// represents blocks event
class BlocksEventObj : public ObjectValue
{
  typedef ObjectValue inherited;
  friend class BlocksView;

  BlocksViewPtr mBlocksView;
public:
  BlocksEventObj(BlocksViewPtr aBlocksView) : mBlocksView(aBlocksView) {};
  virtual void deactivate() P44_OVERRIDE { mBlocksView.reset(); inherited::deactivate(); };
  virtual string getAnnotation() const P44_OVERRIDE { return "blocks status"; };
  virtual TypeInfo getTypeInfo() const P44_OVERRIDE { return inherited::getTypeInfo()|oneshot|freezable; };
//  virtual bool isEventSource() const P44_OVERRIDE { return (bool)mBlocksView; };
//  virtual void registerForFilteredEvents(EventSink* aEventSink, intptr_t aRegId = 0) P44_OVERRIDE
//  {
//    if (mBlocksView) mBlocksView->registerForEvents(aEventSink, aRegId); // no filtering
//  }
};


ScriptObjPtr BlocksView::newBlocksEventObj(bool aLower)
{
  BlockRunner *b = &mActiveBlocks[aLower ? 1 : 0];
  BlocksEventObj* bv = new BlocksEventObj(this);
  bv->setMemberByName("reverse", new BoolValue(aLower));
  if (b->block) {
    bv->setMemberByName("x", new IntegerValue(b->block->mPos.x));
    bv->setMemberByName("y", new IntegerValue(b->block->mPos.y));
    bv->setMemberByName("rotation", new IntegerValue(b->block->mPartRotation));
  }
  if (b->droppedsteps>0) bv->setMemberByName("droppedsteps", new IntegerValue(b->droppedsteps));
  return bv;
}


// event()
static void event_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  EventSource* es = dynamic_cast<EventSource*>(v->blocks().get());
  assert(es);
  f->finish(new OneShotEventNullValue(es, "blocks event"));
}


// show(show [, reverse])
FUNC_ARG_DEFS(show, { numeric }, { numeric|optionalarg } );
static void show_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  if (f->numArgs()==1) {
    v->blocks()->showRunningBlocks(f->arg(0)->boolValue());
  }
  else {
    v->blocks()->showBlock(f->arg(0)->boolValue(), f->arg(1)->boolValue());
  }
  f->finish();
}


// launch(blockNo, column, orientation, interval [, bool reverse])
FUNC_ARG_DEFS(launch, { numeric|text }, { numeric }, { numeric }, { numeric }, { numeric|optionalarg } );
static void launch_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  int blockNo;
  if (f->arg(0)->hasType(text)) {
    string n = f->arg(0)->stringValue();
    for (blockNo = 0; blockNo<Block::numBlockTypes; blockNo++) {
      if (uequals(n, blockDefs[blockNo].name)) break;
    }
  }
  else {
    blockNo = f->arg(0)->intValue();
  }
  bool launched = false;
  if (blockNo>=0 && blockNo<Block::numBlockTypes) {
    launched = v->blocks()->launchBlock(
      static_cast<Block::BlockType>(blockNo),
      v->blocks()->getForegroundColor(),
      f->arg(1)->intValue(), // column
      f->arg(2)->intValue(), // orientation
      f->arg(4)->boolValue(), // reverse
      f->arg(3)->doubleValue()*Second
    );
  }
  f->finish(new BoolValue(launched));
}


// move(dx, dy, rotate [, reverse])
FUNC_ARG_DEFS(move, { numeric }, { numeric }, { numeric|optionalarg }, { numeric|optionalarg } );
static void move_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  PixelPoint dist;
  dist.x = f->arg(0)->intValue();
  dist.y = f->arg(1)->intValue();
  v->blocks()->moveBlock(dist, f->arg(2)->intValue(), f->arg(3)->boolValue());
  f->finish();
}


// drop(dropinterval [, reverse])
FUNC_ARG_DEFS(drop, { numeric }, { numeric|optionalarg } );
static void drop_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  v->blocks()->dropBlock(f->arg(0)->doubleValue()*Second, f->arg(1)->boolValue());
  f->finish();
}


// setdown([reverse])
FUNC_ARG_DEFS(setdown, { numeric|optionalarg } );
static void setdown_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  v->blocks()->setdownBlock(f->arg(0)->boolValue());
  f->finish();
}


// recolor([reverse]);
FUNC_ARG_DEFS(recolor, { numeric|optionalarg } );
static void recolor_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  v->blocks()->recolorBlock(v->blocks()->getForegroundColor(), f->arg(0)->boolValue());
  f->finish();
}


// completedrow([reverse]);
FUNC_ARG_DEFS(completedrow, { numeric|optionalarg } );
static void completedrow_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  int row = v->blocks()->findCompletedRow(f->arg(0)->boolValue());
  if (row<0) f->finish(new AnnotatedNullValue("no completed rows"));
  else f->finish(new NumericValue(row));
}


// removerow(row [, reverse]);
FUNC_ARG_DEFS(removerow, { numeric|optionalarg } );
static void removerow_func(BuiltinFunctionContextPtr f)
{
  BlocksViewObj* v = dynamic_cast<BlocksViewObj*>(f->thisObj().get());
  assert(v);
  v->blocks()->removeRow(f->arg(0)->intValue(), f->arg(1)->boolValue());
  f->finish();
}



//  void removeRow(int aY, bool aBlockFromBottom);



#define ACCESSOR_CLASS BlocksView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  BlocksViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<BlocksViewObj*>(aParentObj.get())->blocks().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_BOOL(Pause);

static const BuiltinMemberDescriptor blocksViewMembers[] = {
  FUNC_DEF_W_ARG(show, executable|null),
  FUNC_DEF_W_ARG(launch, executable|null),
  FUNC_DEF_W_ARG(move, executable|null),
  FUNC_DEF_W_ARG(drop, executable|null),
  FUNC_DEF_W_ARG(setdown, executable|null),
  FUNC_DEF_W_ARG(recolor, executable|null),
  FUNC_DEF_W_ARG(completedrow, executable|numeric),
  FUNC_DEF_W_ARG(removerow, executable|null),
  FUNC_DEF_NOARG(event, executable|objectvalue),
  // property accessors
  ACC_DECL("pause", numeric, Pause),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedBlocksMemberLookupP = NULL;

BlocksViewObj::BlocksViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedBlocksMemberLookupP, blocksViewMembers);
}

#endif // P44SCRIPT_FULL_SUPPORT
