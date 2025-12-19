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

#ifndef __p44lrgraphics_blocksview_hpp__
#define __p44lrgraphics_blocksview_hpp__

#include "p44lrg_common.hpp"

#if P44SCRIPT_FULL_SUPPORT // makes no sense w/o p44Script

#include "canvasview.hpp"

namespace p44 {


  class BlocksView;

  class Block : public P44Obj
  {
    friend class BlocksView;

    BlocksView &mBlocksView;

  public:
  
    typedef enum {
      block_line,
      block_sqare,
      block_l,
      block_l_reverse,
      block_t,
      block_squiggly,
      block_squiggly_reverse,
      numBlockTypes
    } BlockType;

    PixelPoint mPos; ///< coordinates of the (rotation) center of the piece
    int mPartRotation; ///< 0=normal, 1..3=90 degree rotation steps clockwise
    bool mShown; ///< set when piece is currently shown on playfield
    BlockType mBlockType; ///< the block type
    PixelColor mColor;

    Block(BlocksView& aBlocksView,  BlockType aBlockType, PixelColor aColor);

    /// show on or hide from playfield
    void show(bool aShow);

    /// move block
    bool move(PixelPoint aDistance, int aRot, bool aOpenAtBottom);

    /// (re)position piece on playfield, returns true if possible, false if not
    /// @param aPt playfield coordinates of where piece should be positioned
    /// @param aOrientation new orientation
    /// @param aOpenAtBottom if set, playfield is considered open at the bottom
    /// @return true if positioning could be carried out (was possible without collision)
    bool position(PixelPoint aPt, int aOrientation, bool aOpenAtBottom);

    /// get max extents of block around its center
    void getExtents(int aOrientation, PixelCoord& aMinDx, PixelCoord& aMaxDx, PixelCoord& aMinDy, PixelCoord& aMaxDy);

  protected:

    /// iterate over pixels of this piece
    /// @param aPixelIndex enumerate pixels from 0..numPixels
    /// @param aPt returns coordinates relative to piece's rotation center
    /// @return false when piece has no more pixels
    bool getPixel(int aPixelIndex, PixelPoint& aPt);

  private:

    /// transform relative to absolute pixel position
    /// @param aPixelIndex enumerate pixels from 0..numPixels
    /// @param aCenter playfield coordinates of piece's center
    /// @param aOrientation new orientation
    /// @param aPt returns pixel's playfield coordinates (no playfield bounds checking, can be negative)
    /// @return false when piece has no more pixels
    bool getPositionedPixel(int aPixelIndex, PixelPoint aCenter, int aOrientation, PixelPoint& aPt);

  };
  typedef boost::intrusive_ptr<Block> BlockPtr;



  class BlockRunner
  {
  public:
    BlockPtr block;
    MLMicroSeconds lastStep;
    MLMicroSeconds stepInterval;
    bool movingUp;
    bool dropping; ///< set when block starts dropping
    int droppedsteps; ///< how many steps block has dropped so far
  };


  class BlocksView : public CanvasView, public P44Script::EventSource
  {
    typedef CanvasView inherited;
    friend class Block;

    BlockRunner mActiveBlocks[2]; ///< the max 2 active blocks (top and bottom)
    bool mPause;
    MLMicroSeconds mNextCalculation;

  public:

    BlocksView();
    virtual ~BlocksView();

    static const char* staticTypeName() { return "blocks"; };
    static P44View* newInstance() { return new BlocksView; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;

    /// @return ScriptObj representing current status
    P44Script::ScriptObjPtr newBlocksEventObj(bool aLower);

    /// clear contents of this view
    /// @note base class just resets content size to zero, subclasses might NOT want to do that
    ///   and thus choose NOT to call inherited.
    virtual void clear() P44_OVERRIDE;

    /// stop all animations
    virtual void stopAnimations() P44_OVERRIDE;

    /// calculate changes on the display for a given time, return time of when the NEXT step should be shown
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise time of when we need to SHOW
    ///   the next result, i.e. should have called step() and display rendering already.
    /// @note all stepping calculations will be exclusively based on aStepShowTime, and never real time, so
    ///   we can calculate results in advance
    virtual MLMicroSeconds stepInternal(MLMicroSeconds aPriorityUntil) P44_OVERRIDE;

    /// launch a block into the playfield
    /// @param aBlockType the type of block
    /// @param aColumn the column where to launch (will be adjusted left or right in case block extends playfield)
    /// @param aOrientation initial orientation
    /// @param aBottom if set, launch a block from bottom up
    bool launchBlock(Block::BlockType aBlockType, PixelColor aColor, int aColumn, int aOrientation, bool aBottom, MLMicroSeconds aStepInterval);

    /// find completed row
    /// @param aBlockFromBottom set to check for rows completed by blocks moving up
    /// @return negative if no completed row was found, y coordinate of completed row otherwise
    int findCompletedRow(bool aBlockFromBottom);

    /// remove a row
    void removeRow(int aY, bool aBlockFromBottom);

    /// show or hide the running blocks
    /// @param aShow set to show the running blocks
    void showRunningBlocks(bool aShow);

    /// show or hide the running blocks
    /// @param aShow set to show blocks
    /// @param aLower address the block "falling up"
    void showBlock(bool aShow, bool aLower);

    /// set down the block, which means detaching it from the runner
    /// @param aLower address the block "falling up"
    void setdownBlock(bool aLower);

    /// show or hide the running blocks
    /// @param aColor new Color
    /// @param aLower address the block "falling up"
    void recolorBlock(PixelColor aColor, bool aLower);

    /// move block
    /// @param aLower address the block "falling up"
    void moveBlock(PixelPoint aDistance, int aRot, bool aLower);

    /// position block
    /// @param aPosition new position, <0 for a coordinate means no change
    /// @param aRotation new rotation, <0 means no change
    /// @param aLower address the block "falling up"
    void positionBlock(PixelPoint aPosition, int aRotation, bool aLower);

    /// set new speed for a running block
    /// @param aInterval new falling speed
    /// @param aLower address the block "falling up"
    void setBlockInterval(MLMicroSeconds aInterval, bool aLower);

    /// change speed of block
    /// @param aLower address the block "falling up"
    void dropBlock(MLMicroSeconds aDropStepInterval, bool aLower);

    /// @name trivial property getters/setters
    /// @{
    bool getPause() { return mPause; }
    void setPause(bool aValue) { mPause = aValue; }
    /// @}

  private:

    bool isWithinPlayfield(PixelPoint aPt, bool aOpen, bool aOpenAtBottom);
    void checkRows(bool aBlockFromBottom, int aRemovedRows);

  };
  typedef boost::intrusive_ptr<BlocksView> BlocksViewPtr;


  namespace P44Script {

    /// represents a ViewScroller
    class BlocksViewObj : public CanvasViewObj
    {
      typedef CanvasViewObj inherited;
    public:
      BlocksViewObj(P44ViewPtr aView);
      BlocksViewPtr blocks() { return boost::static_pointer_cast<BlocksView>(inherited::view()); };
    };

  } // namespace P44Script


} // namespace p44

#endif // P44SCRIPT_FULL_SUPPORT
#endif /* __p44lrgraphics_blocksview_hpp__ */
