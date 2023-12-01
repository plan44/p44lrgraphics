//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2017-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "lifeview.hpp"
#include "utils.hpp"

using namespace p44;


// MARK: ===== LifeView


LifeView::LifeView() :
  mGenerationInterval(777*MilliSecond),
  mLastGeneration(Never),
  mStaticcount(0),
  mMaxStatic(23),
  mMinStatic(10),
  mMinPopulation(9)
{
  // original coloring scheme was yellow-green for newborn cells
  setForegroundColor({ 128, 255, 0, 0 });
  mHueGradient = -330.0/360*100;
  mSatGradient = 0;
  mBriGradient = -25;
}

LifeView::~LifeView()
{
}


void LifeView::clear()
{
  stopAnimations();
  mCells.clear();
  prepareCells();
}


bool LifeView::prepareCells()
{
  PixelPoint csz = getContentSize();
  int numCells = csz.x*csz.y;
  if (numCells<=0) return false;
  if (numCells!=mCells.size()) {
    mCells.clear();
    for (int i=0; i<numCells; ++i) {
      mCells.push_back(0);
    }
    makeDirty();
  }
  return true;
}



void LifeView::setGenerationInterval(MLMicroSeconds aInterval)
{
  mGenerationInterval = aInterval;
}


#define MIN_SPAWN_START 3


MLMicroSeconds LifeView::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil, aNow);
  if (mAlpha>0 && mGenerationInterval!=Never && aNow>=mLastGeneration+mGenerationInterval) {
    mLastGeneration = aNow;
    nextGeneration();
    updateNextCall(nextCall, aNow+mGenerationInterval);
  }
  return nextCall;
}


void LifeView::recalculateColoring()
{
  pixelToHsb(mForegroundColor, mHue, mSaturation, mBrightness);
  inherited::recalculateColoring();
}


void LifeView::nextGeneration()
{
  calculateGeneration();
  if (mDynamics==0) {
    mStaticcount++;
    LOG(LOG_NOTICE, "No dynamics for %d cycles, population is %d", mStaticcount, mPopulation);
    if (mStaticcount>mMaxStatic || (mPopulation<mMinPopulation && mStaticcount>mMinStatic)) {
      revive();
    }
  }
  else {
    if (mStaticcount>0) {
      mStaticcount -= 2;
      if (mStaticcount<0) mStaticcount = 0;
      LOG(LOG_NOTICE, "Dynamics (%+d) in this cycle, population is %d, reduced staticount to %d", mDynamics, mPopulation, mStaticcount);
    }
    else {
      LOG(LOG_INFO, "Dynamics (%+d) in this cycle, population is %d", mDynamics, mPopulation);
    }
  }
  makeDirty();
}


void LifeView::revive()
{
  // shoot in some new cells
  createRandomCells((int)mCells.size()/20+1,(int)mCells.size()/6+1);
  makeDirty();
}



int LifeView::cellindex(int aX, int aY, bool aWrap)
{
  if (aX<0) {
    if (!aWrap) return (int)mCells.size(); // out of range
    aX += mContent.dx;
  }
  else if (aX>=mContent.dx) {
    if (!aWrap) return (int)mCells.size(); // out of range
    aX -= mContent.dx;
  }
  if (aY<0) {
    if (!aWrap) return (int)mCells.size(); // out of range
    aY += mContent.dy;
  }
  else if (aY>=mContent.dx) {
    if (!aWrap) return (int)mCells.size(); // out of range
    aY -= mContent.dy;
  }
  return aY*mContent.dx+aX;
}


void LifeView::calculateGeneration()
{
  mPopulation = 0;
  mDynamics = 0;
  if (!prepareCells()) return;
  // cell age 0 : dead for longer
  // cell age 1 : killed in this cycle
  // cell age 2...n : living, with
  // - 2 = created out of void
  // - 3 = spawned
  // - 4..n aged
  // first age all living cells by 1, clean those that were killed in last cycle
  for (int i=0; i<mCells.size(); ++i) {
    int age = mCells[i];
    if (age==1) {
      mCells[i] = 0;
    }
    else if (age==2) {
      mCells[i] = 4; // skip 3
    }
    else if (age>2) {
      mCells[i]++; // just age normally
    }
  }
  // apply rules
  for (int x=0; x<mContent.dx; ++x) {
    for (int y=0; y<mContent.dy; y++) {
      int ci = cellindex(x, y, false);
      // calculate number of neighbours
      int nn = 0;
      for (int dx=-1; dx<2; dx+=1) {
        for (int dy=-1; dy<2; dy+=1) {
          if (dx!=0 || dy!=0) {
            // one of the 8 neighbours
            int nci = cellindex(x+dx, y+dy, true);
            if (nci<mCells.size() && (mCells[nci]>3 || mCells[nci]==1)) {
              // is a living neighbour (exclude newborns of this cycle, include those that will die at end of this cycle
              nn++;
            }
          }
        }
      }
      // rules for living cells:
      if (mCells[ci]>0) {
        // - Any live cell with fewer than two live neighbours dies,
        //   as if caused by underpopulation.
        // - Any live cell with more than three live neighbours dies, as if by overpopulation.
        if (nn<2 || nn>3) {
          mCells[ci] = 1; // will die at end of this cycle (but still alive for calculation)
          mDynamics--; // a dying cell is a transition
        }
        else {
          mPopulation++; // lives on, counts for population
        }
        // - Any live cell with two or three live neighbours lives on to the next generation.
      }
      // rule for dead cells:
      else {
        // - Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
        if (nn==3) {
          mCells[ci] = 3; // spawned
          mDynamics++; // a new cell is a transition
          mPopulation++; // now lives, counts for population
        }
      }
    }
  }
}


void LifeView::createRandomCells(int aMinCells, int aMaxCells)
{
  if (!prepareCells()) return;
  int numcells = aMinCells + rand() % (aMaxCells-aMinCells+1);
  while (numcells-- > 0) {
    int ci = rand() % mCells.size();
    mCells[ci] = 2; // created out of void
  }
  makeDirty();
}


typedef struct {
  int x;
  int y;
} PixPos;

typedef struct {
  int numpix;
  const PixPos *pixels;
} PixPattern;


static const PixPos blinker[] = { {0,-1}, {0,0}, {0,1} };
static const PixPos toad[] = { {1,0}, {2,0}, {3,0}, {0,1}, {1,1}, {2,1} };
static const PixPos beacon[] = { {0,0}, {1,0}, {0,1}, {3,2}, {2,3}, {3,3} };
static const PixPos pentadecathlon[] = {
  {-1,-5}, {0,-5}, {1,-5},
  {0,-4},
  {0,-3},
  {-1,-2}, {0,-2}, {1,-2},
  {-1,0}, {0,0}, {1,0},
  {-1,1}, {0,1}, {1,1},
  {-1,3}, {0,3}, {1,3},
  {0,4},
  {0,5},
  {-1,6}, {0,6}, {1,6}
};
static const PixPos rpentomino[] = { {0,-1}, {1,-1}, {-1,0}, {0,0}, {1,1} };
static const PixPos diehard[] = { {-3,0}, {-2,0}, {-2,1}, {2,1}, {3,-1}, {3,1}, {4,1} };
static const PixPos acorn[] = { {-3,1}, {-2,-1}, {-2,1}, {0,0}, {1,1}, {2,1}, {3,1} };
static const PixPos glider[] = { {0,-1}, {1,0}, {-1,1}, {0,1}, {1,1} };

#define PAT(x) { sizeof(x)/sizeof(PixPos), x }


static const PixPattern patterns[] = {
  PAT(blinker),
  PAT(toad),
  PAT(beacon),
  PAT(pentadecathlon),
  PAT(rpentomino),
  PAT(diehard),
  PAT(acorn),
  PAT(glider)
};
#define NUMPATTERNS (sizeof(patterns)/sizeof(PixPattern))


void LifeView::placePattern(uint16_t aPatternNo, bool aWrap, int aCenterX, int aCenterY, int aOrientation)
{
  if (!prepareCells()) return;
  if (aPatternNo>=NUMPATTERNS) return;
  if (aCenterX<0) aCenterX = rand() % mContent.dx;
  if (aCenterY<0) aCenterY = rand() % mContent.dy;
  if (aOrientation<0) aOrientation = rand() % 4;
  for (int i=0; i<patterns[aPatternNo].numpix; i++) {
    int x,y;
    PixPos px = patterns[aPatternNo].pixels[i];
    switch (aOrientation) {
      default: x=aCenterX+px.x; y=aCenterY+px.y; break;
      case 1: x=aCenterX+px.y; y=aCenterY-px.x; break;
      case 2: x=aCenterX-px.x; y=aCenterY-px.y; break;
      case 3: x=aCenterX-px.y; y=aCenterY+px.x; break;
    }
    int ci = cellindex(x, y, aWrap);
    mCells[ci] = 2; // created out of void
  }
}


PixelColor LifeView::contentColorAt(PixelPoint aPt)
{
  PixelColor pix = transparent;
  // simplest colorisation: from yellow (young) to red
  int ci = cellindex(aPt.x, aPt.y, false);
  if (ci>=mCells.size()) return pix; // out of range
  pix = mBackgroundColor;
  int age = mCells[ci];
  if (age<2) return pix; // dead, background
  // fixed, not dependent on color set
  if (age==2) {
    // artificially created
    pix.b = 255;
    pix.r = 200;
    pix.g = 220;
    return pix;
  }
  // dependent on foreground color
  double cellHue;
  double cellBrightness;
  double cellSaturation;
  if (age==3) {
    // just born - leave the color as-is
    cellHue = mHue;
    cellBrightness = 1.0;
    cellSaturation = mSaturation;
  }
  else {
    age -= 3;
    if (age>57) age = 57; // limit
    cellHue = cyclic(mHue+mHueGradient*660/7.3+(mHueGradient*660/66*age),0,360);
    if (cellHue<0) cellHue+=360;
    cellBrightness = limited(mBrightness+(mBriGradient/10*age),0,1);
    cellSaturation = limited(mSaturation+(mSatGradient/10*age),0,1);
  }
  pix = hsbToPixel(cellHue, cellSaturation, cellBrightness);
  return pix;
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr LifeView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    #if !ENABLE_P44SCRIPT
    if (aViewConfig->get("generationinterval", o)) {
      mGenerationInterval  = o->doubleValue()*Second;
    }
    if (aViewConfig->get("maxstatic", o)) {
      mMaxStatic  = o->int32Value();
    }
    if (aViewConfig->get("minstatic", o)) {
      mMinStatic  = o->int32Value();
    }
    if (aViewConfig->get("minpopulation", o)) {
      mMinPopulation  = o->int32Value();
    }
    #endif // !ENABLE_P44SCRIPT
    if (aViewConfig->get("addrandom", o)) {
      int c = o->int32Value();
      createRandomCells(c/3, c);
    }
    if (aViewConfig->get("addpattern", o)) {
      int p = o->int32Value();
      placePattern(p);
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

/// @return the current status of the view, in the same format as accepted by configure()
JsonObjectPtr LifeView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("generationinterval", JsonObject::newDouble(getGenerationIntervalS());
  status->add("maxstatic", JsonObject::newInt32(getMaxStatic()));
  status->add("minstatic", JsonObject::newInt32(getMinStatic()));
  status->add("minpopulation", JsonObject::newInt32(getMinPopulation()));
  return status;
}

#endif // ENABLE_VIEWSTATUS

#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr LifeView::newViewObj()
{
  // base class with standard functionality
  return new LifeViewObj(this);
}


#define ACCESSOR_CLASS LifeView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  LifeViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<LifeViewObj*>(aParentObj.get())->life().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_DBL(GenerationIntervalS);
ACC_IMPL_INT(MaxStatic);
ACC_IMPL_INT(MinStatic);
ACC_IMPL_INT(MinPopulation);

static const BuiltinMemberDescriptor lifeMembers[] = {
  // property accessors
  ACC_DECL("generationinterval", numeric|lvalue, GenerationIntervalS),
  ACC_DECL("maxstatic", numeric|lvalue, MaxStatic),
  ACC_DECL("minstatic", numeric|lvalue, MinStatic),
  ACC_DECL("minpopulation", numeric|lvalue, MinPopulation),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedLifeMemberLookupP = NULL;

LifeViewObj::LifeViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  if (sharedLifeMemberLookupP==NULL) {
    sharedLifeMemberLookupP = new BuiltInMemberLookup(lifeMembers);
    sharedLifeMemberLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedLifeMemberLookupP);
}

#endif // ENABLE_P44SCRIPT
