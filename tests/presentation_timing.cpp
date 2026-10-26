// SPDX-License-Identifier: GPL-3.0-or-later
// Build with -DP44LRG_TIMING_TESTS=ON, then run p44lrg_timing_test.
// Deterministic tests: show times deliberately differ from the real clock.
#include "viewstack.hpp"
#include "viewscroller.hpp"
#include "torchview.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace p44;

static void require(bool condition, const char* message)
{
  if (!condition) { std::cerr << message << std::endl; std::exit(1); }
}

static const MLMicroSeconds epoch = 10*Second;

class TimingView : public P44View
{
public:
  MLMicroSeconds calculation = Infinite;
  bool change = false;
protected:
  StepResult stepInternal(MLMicroSeconds priorityUntil) P44_OVERRIDE
  {
    if (change) makeDirty();
    StepResult result;
    updateNextCall(result, calculation, priorityUntil);
    return result;
  }
};

static void torchAndScroller(bool reverse)
{
  ViewStackPtr root = new ViewStack;
  ViewScrollerPtr scroller = new ViewScroller;
  TorchViewPtr torch = new TorchView;
  P44ViewPtr content = new P44View;
  scroller->setFrame({0,0,20,7});
  content->setFrame({0,0,100,7});
  torch->setFrame({0,0,2,7});
  torch->setContent({0,0,2,7});
  scroller->setScrolledView(content);
  if (reverse) { root->pushView(torch); root->pushView(scroller); }
  else { root->pushView(scroller); root->pushView(torch); }
  root->makeRootView(true);
  scroller->startScroll(1,0,40*MilliSecond,false,-1,epoch);
  root->updated();
  std::vector<MLMicroSeconds> shows, steps;
  for (MLMicroSeconds now=epoch; now<epoch+400*MilliSecond;) {
    steps.push_back(now-epoch);
    StepResult next = root->viewRoot()->stepHierarchy(now,now+50*MilliSecond,now-2*MilliSecond);
    if (root->dirtyAt(now)) { shows.push_back(now-epoch); root->updated(); }
    require(next.nextCalculation>now, "step did not advance");
    now=next.nextCalculation;
  }
  require(shows.size()==10, "wrong scroller presentation count");
  for (size_t i=0;i<shows.size();++i) require(shows[i]==i*40*MilliSecond, "Torch disturbed scroller cadence");
  for (int i=0;i<16;++i) {
    bool found=false;
    for (auto step : steps) if (step==i*25*MilliSecond) found=true;
    require(found, "Torch calculation deadline was lost");
  }
}

static void boundedMask()
{
  boost::intrusive_ptr<TimingView> view = new TimingView;
  view->makeRootView(true);
  view->setLocalTimingPriority(true);
  view->calculation=epoch+Second;
  view->change=true;
  auto result=view->viewRoot()->stepHierarchy(epoch,epoch+50*MilliSecond,epoch-Second);
  require(result.nextCalculation==epoch+50*MilliSecond, "mask expiry needs a wakeup");
  require(!view->dirtyAt(epoch+49*MilliSecond), "mask expired early");
  view->viewRoot()->stepHierarchy(epoch+25*MilliSecond,epoch+75*MilliSecond,epoch);
  require(view->dirtyAt(epoch+50*MilliSecond), "repeated changes extended pending mask");
  view->updated();
  require(!view->dirtyAt(epoch+Second), "updated did not clear dirty state");
}

static void torchAlone()
{
  TorchViewPtr torch = new TorchView;
  torch->setFrame({0,0,2,7});
  torch->setContent({0,0,2,7});
  torch->makeRootView(true);
  torch->updated();
  for (int i=0;i<10;++i) {
    MLMicroSeconds now=epoch+i*25*MilliSecond;
    auto result=torch->viewRoot()->stepHierarchy(now,now+50*MilliSecond,now);
    require(torch->dirtyAt(now), "Torch without a timing neighbor was suppressed");
    require(result.nextCalculation==now+25*MilliSecond, "wrong standalone Torch deadline");
    torch->updated();
  }
}

static void externalWakeup()
{
  P44ViewPtr view = new P44View;
  view->makeRootView(true);
  view->updated();
  int wakeups=0;
  auto& loop=MainLoop::currentMainLoop();
  view->viewRoot()->setNeedStepCB([&](MLMicroSeconds) { ++wakeups; loop.terminate(0); });
  loop.startupMainLoop(true);
  view->setAlpha(128);
  loop.mainLoopCycle();
  loop.finalizeMainLoop();
  require(wakeups>0, "external mutation did not wake a static scene");
  require(view->dirtyAt(epoch), "external mutation did not remain dirty");
}

static void parentPriority()
{
  ViewScrollerPtr parent = new ViewScroller;
  boost::intrusive_ptr<TimingView> child = new TimingView;
  child->setAlignAnimationSteps(false); // child explicitly wants immediate output
  child->change=true;
  parent->setScrolledView(child);
  parent->makeRootView(true);
  parent->startScroll(1,0,40*MilliSecond,false,-1,epoch);
  parent->viewRoot()->stepHierarchy(epoch,epoch+50*MilliSecond,epoch);
  parent->updated();
  parent->viewRoot()->stepHierarchy(epoch+25*MilliSecond,epoch+75*MilliSecond,epoch);
  require(!parent->dirtyAt(epoch+25*MilliSecond), "child overrode parent timing priority");
  require(parent->dirtyAt(epoch+40*MilliSecond), "parent lost pending child changes");
}

static void animatorDeadline(bool align, MLMicroSeconds internalDelay, MLMicroSeconds expectedDelay)
{
  boost::intrusive_ptr<TimingView> view = new TimingView;
  view->makeRootView(true);
  view->setAlignAnimationSteps(align);
  view->animatorFor("alpha")->from(255)->stepParams(10*MilliSecond)->animate(0,Second);
  // ValueAnimator starts on the real clock; subsequent steps use the supplied show time.
  MLMicroSeconds now=MainLoop::now()+MilliSecond;
  if (DEFINED_INTERVAL(internalDelay)) view->calculation=now+internalDelay;
  auto result=view->viewRoot()->stepHierarchy(now,now+50*MilliSecond,now-MilliSecond);
  require(result.nextCalculation==now+expectedDelay, "animator deadline incorrectly omitted");
}

int main()
{
  torchAndScroller(false);
  torchAndScroller(true);
  torchAlone();
  boundedMask();
  parentPriority();
  animatorDeadline(true,20*MilliSecond,20*MilliSecond);
  animatorDeadline(false,20*MilliSecond,10*MilliSecond);
  animatorDeadline(true,100*MilliSecond,10*MilliSecond);
  animatorDeadline(true,Infinite,10*MilliSecond);
  externalWakeup();
  std::cout << "Presentation timing tests passed" << std::endl;
}
