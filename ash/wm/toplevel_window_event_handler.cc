// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/toplevel_window_event_handler.h"

#include "ash/shell.h"
#include "ash/wm/resize_shadow_controller.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_observer.h"
#include "ash/wm/window_util.h"
#include "ash/wm/workspace/snap_sizer.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_observer.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/hit_test.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/events/gestures/gesture_recognizer.h"
#include "ui/gfx/screen.h"

namespace {
const double kMinHorizVelocityForWindowSwipe = 1100;
const double kMinVertVelocityForWindowMinimize = 1000;
}

namespace ash {

namespace {

gfx::Point ConvertPointToParent(aura::Window* window,
                                const gfx::Point& point) {
  gfx::Point result(point);
  aura::Window::ConvertPointToTarget(window, window->parent(), &result);
  return result;
}

}  // namespace

// ScopedWindowResizer ---------------------------------------------------------

// Wraps a WindowResizer and installs an observer on its target window.  When
// the window is destroyed ResizerWindowDestroyed() is invoked back on the
// ToplevelWindowEventHandler to clean up.
class ToplevelWindowEventHandler::ScopedWindowResizer
    : public aura::WindowObserver,
      public wm::WindowStateObserver {
 public:
  ScopedWindowResizer(ToplevelWindowEventHandler* handler,
                      WindowResizer* resizer);
  virtual ~ScopedWindowResizer();

  WindowResizer* resizer() { return resizer_.get(); }

  // WindowObserver overrides:
  virtual void OnWindowHierarchyChanging(
      const HierarchyChangeParams& params) OVERRIDE;
  virtual void OnWindowDestroying(aura::Window* window) OVERRIDE;

  // WindowStateObserver overrides:
  virtual void OnWindowShowTypeChanged(wm::WindowState* window_state,
                                       wm::WindowShowType type) OVERRIDE;

 private:
  void AddHandlers(aura::Window* container);
  void RemoveHandlers();

  ToplevelWindowEventHandler* handler_;
  scoped_ptr<WindowResizer> resizer_;

  // If not NULL, this is an additional container that the dragged window has
  // moved to which ScopedWindowResizer has temporarily added observers on.
  aura::Window* target_container_;

  DISALLOW_COPY_AND_ASSIGN(ScopedWindowResizer);
};

ToplevelWindowEventHandler::ScopedWindowResizer::ScopedWindowResizer(
    ToplevelWindowEventHandler* handler,
    WindowResizer* resizer)
    : handler_(handler),
      resizer_(resizer),
      target_container_(NULL)  {
  resizer_->GetTarget()->AddObserver(this);
  wm::GetWindowState(resizer_->GetTarget())->AddObserver(this);
}

ToplevelWindowEventHandler::ScopedWindowResizer::~ScopedWindowResizer() {
  RemoveHandlers();
  resizer_->GetTarget()->RemoveObserver(this);
  wm::GetWindowState(resizer_->GetTarget())->RemoveObserver(this);
}

void ToplevelWindowEventHandler::ScopedWindowResizer::OnWindowHierarchyChanging(
    const HierarchyChangeParams& params) {
  if (params.receiver != resizer_->GetTarget())
    return;
  wm::WindowState* state = wm::GetWindowState(params.receiver);
  if (state->continue_drag_after_reparent()) {
    state->set_continue_drag_after_reparent(false);
    AddHandlers(params.new_parent);
  } else {
    handler_->CompleteDrag(DRAG_COMPLETE);
  }
}

void ToplevelWindowEventHandler::ScopedWindowResizer::OnWindowShowTypeChanged(
    wm::WindowState* window_state,
    wm::WindowShowType old) {
  if (!window_state->IsNormalShowState())
    handler_->CompleteDrag(DRAG_COMPLETE);
}

void ToplevelWindowEventHandler::ScopedWindowResizer::OnWindowDestroying(
    aura::Window* window) {
  DCHECK_EQ(resizer_->GetTarget(), window);
  handler_->ResizerWindowDestroyed();
}

void ToplevelWindowEventHandler::ScopedWindowResizer::AddHandlers(
    aura::Window* container) {
  RemoveHandlers();
  if (!handler_->owner()->Contains(container)) {
    container->AddPreTargetHandler(handler_);
    container->AddPostTargetHandler(handler_);
    target_container_ = container;
  }
}

void ToplevelWindowEventHandler::ScopedWindowResizer::RemoveHandlers() {
  if (target_container_) {
    target_container_->RemovePreTargetHandler(handler_);
    target_container_->RemovePostTargetHandler(handler_);
    target_container_ = NULL;
  }
}


// ToplevelWindowEventHandler --------------------------------------------------

ToplevelWindowEventHandler::ToplevelWindowEventHandler(aura::Window* owner)
    : owner_(owner),
      in_move_loop_(false),
      drag_reverted_(false),
      in_gesture_drag_(false),
      destroyed_(NULL) {
  aura::client::SetWindowMoveClient(owner, this);
  Shell::GetInstance()->display_controller()->AddObserver(this);
  owner->AddPreTargetHandler(this);
  owner->AddPostTargetHandler(this);
}

ToplevelWindowEventHandler::~ToplevelWindowEventHandler() {
  Shell::GetInstance()->display_controller()->RemoveObserver(this);
  if (destroyed_)
    *destroyed_ = true;
}

void ToplevelWindowEventHandler::OnKeyEvent(ui::KeyEvent* event) {
  if (window_resizer_.get() && event->type() == ui::ET_KEY_PRESSED &&
      event->key_code() == ui::VKEY_ESCAPE) {
    CompleteDrag(DRAG_REVERT);
  }
}

void ToplevelWindowEventHandler::OnMouseEvent(
    ui::MouseEvent* event) {
  if ((event->flags() &
      (ui::EF_MIDDLE_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON)) != 0)
    return;

  if (in_gesture_drag_)
    return;

  aura::Window* target = static_cast<aura::Window*>(event->target());
  switch (event->type()) {
    case ui::ET_MOUSE_PRESSED:
      HandleMousePressed(target, event);
      break;
    case ui::ET_MOUSE_DRAGGED:
      HandleDrag(target, event);
      break;
    case ui::ET_MOUSE_CAPTURE_CHANGED:
    case ui::ET_MOUSE_RELEASED:
      HandleMouseReleased(target, event);
      break;
    case ui::ET_MOUSE_MOVED:
      HandleMouseMoved(target, event);
      break;
    case ui::ET_MOUSE_EXITED:
      HandleMouseExited(target, event);
      break;
    default:
      break;
  }
}

void ToplevelWindowEventHandler::OnGestureEvent(ui::GestureEvent* event) {
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (!target->delegate())
    return;

  if (window_resizer_.get() && !in_gesture_drag_)
    return;

  if (window_resizer_.get() &&
      window_resizer_->resizer()->GetTarget() != target) {
    return;
  }

  switch (event->type()) {
    case ui::ET_GESTURE_TAP_DOWN: {
      int component =
          target->delegate()->GetNonClientComponent(event->location());
      if (!(WindowResizer::GetBoundsChangeForWindowComponent(component) &
            WindowResizer::kBoundsChange_Resizes))
        return;
      internal::ResizeShadowController* controller =
          Shell::GetInstance()->resize_shadow_controller();
      if (controller)
        controller->ShowShadow(target, component);
      return;
    }
    case ui::ET_GESTURE_END: {
      internal::ResizeShadowController* controller =
          Shell::GetInstance()->resize_shadow_controller();
      if (controller)
        controller->HideShadow(target);
      return;
    }
    case ui::ET_GESTURE_SCROLL_BEGIN: {
      if (in_gesture_drag_)
        return;
      int component =
          target->delegate()->GetNonClientComponent(event->location());
      if (WindowResizer::GetBoundsChangeForWindowComponent(component) == 0)
        return;
      gfx::Point location_in_parent(
          ConvertPointToParent(target, event->location()));
      AttemptToStartDrag(target, location_in_parent, component,
                         aura::client::WINDOW_MOVE_SOURCE_TOUCH);
      event->StopPropagation();
      return;
    }
    default:
      break;
  }

  if (!in_gesture_drag_)
    return;

  switch (event->type()) {
    case ui::ET_GESTURE_SCROLL_UPDATE: {
      HandleDrag(target, event);
      event->StopPropagation();
      return;
    }
    case ui::ET_GESTURE_SCROLL_END:
      CompleteDrag(DRAG_COMPLETE);
      event->StopPropagation();
      return;
    case ui::ET_SCROLL_FLING_START: {
      CompleteDrag(DRAG_COMPLETE);

      int component =
          target->delegate()->GetNonClientComponent(event->location());
      if (component != HTCAPTION)
        return;

      wm::WindowState* window_state = wm::GetWindowState(target);
      if (!window_state->IsNormalShowState())
        return;

      if (event->details().velocity_y() > kMinVertVelocityForWindowMinimize) {
        SetWindowShowTypeFromGesture(target, wm::SHOW_TYPE_MINIMIZED);
      } else if (event->details().velocity_y() <
                 -kMinVertVelocityForWindowMinimize) {
        SetWindowShowTypeFromGesture(target, wm::SHOW_TYPE_MAXIMIZED);
      } else if (event->details().velocity_x() >
                 kMinHorizVelocityForWindowSwipe) {
        SetWindowShowTypeFromGesture(target, wm::SHOW_TYPE_RIGHT_SNAPPED);
      } else if (event->details().velocity_x() <
                 -kMinHorizVelocityForWindowSwipe) {
        SetWindowShowTypeFromGesture(target, wm::SHOW_TYPE_LEFT_SNAPPED);
      }
      event->StopPropagation();
      return;
    }
    default:
      return;
  }
}

aura::client::WindowMoveResult ToplevelWindowEventHandler::RunMoveLoop(
    aura::Window* source,
    const gfx::Vector2d& drag_offset,
    aura::client::WindowMoveSource move_source) {
  DCHECK(!in_move_loop_);  // Can only handle one nested loop at a time.
  aura::Window* root_window = source->GetRootWindow();
  DCHECK(root_window);
  gfx::Point drag_location;
  if (move_source == aura::client::WINDOW_MOVE_SOURCE_TOUCH &&
      aura::Env::GetInstance()->is_touch_down()) {
    bool has_point = ui::GestureRecognizer::Get()->
        GetLastTouchPointForTarget(source, &drag_location);
    DCHECK(has_point);
  } else {
    drag_location = root_window->GetDispatcher()->GetLastMouseLocationInRoot();
    aura::Window::ConvertPointToTarget(
        root_window, source->parent(), &drag_location);
  }
  // Set the cursor before calling AttemptToStartDrag(), as that will
  // eventually call LockCursor() and prevent the cursor from changing.
  aura::client::CursorClient* cursor_client =
      aura::client::GetCursorClient(root_window);
  if (cursor_client)
    cursor_client->SetCursor(ui::kCursorPointer);
  AttemptToStartDrag(source, drag_location, HTCAPTION, move_source);

  in_move_loop_ = true;
  bool destroyed = false;
  destroyed_ = &destroyed;
  base::MessageLoopForUI* loop = base::MessageLoopForUI::current();
  base::MessageLoop::ScopedNestableTaskAllower allow_nested(loop);
  base::RunLoop run_loop(aura::Env::GetInstance()->GetDispatcher());
  quit_closure_ = run_loop.QuitClosure();
  run_loop.Run();
  if (destroyed)
    return aura::client::MOVE_CANCELED;
  destroyed_ = NULL;
  in_move_loop_ = false;
  return drag_reverted_ ? aura::client::MOVE_CANCELED :
      aura::client::MOVE_SUCCESSFUL;
}

void ToplevelWindowEventHandler::EndMoveLoop() {
  if (in_move_loop_)
    CompleteDrag(DRAG_REVERT);
}

void ToplevelWindowEventHandler::OnDisplayConfigurationChanging() {
  CompleteDrag(DRAG_REVERT);
}

void ToplevelWindowEventHandler::AttemptToStartDrag(
    aura::Window* window,
    const gfx::Point& point_in_parent,
    int window_component,
    aura::client::WindowMoveSource source) {
  if (window_resizer_.get())
    CompleteDrag(DRAG_REVERT);
  WindowResizer* resizer = CreateWindowResizer(window, point_in_parent,
      window_component, source).release();
  if (!resizer)
    return;

  window_resizer_.reset(new ScopedWindowResizer(this, resizer));

  pre_drag_window_bounds_ = window->bounds();
  in_gesture_drag_ = (source == aura::client::WINDOW_MOVE_SOURCE_TOUCH);
}

void ToplevelWindowEventHandler::CompleteDrag(DragCompletionStatus status) {
  scoped_ptr<ScopedWindowResizer> resizer(window_resizer_.release());
  if (resizer) {
    if (status == DRAG_COMPLETE)
      resizer->resizer()->CompleteDrag();
    else
      resizer->resizer()->RevertDrag();
  }
  drag_reverted_ = (status == DRAG_REVERT);

  in_gesture_drag_ = false;
  if (in_move_loop_)
    quit_closure_.Run();
}

void ToplevelWindowEventHandler::HandleMousePressed(
    aura::Window* target,
    ui::MouseEvent* event) {
  // Move/size operations are initiated post-target handling to give the target
  // an opportunity to cancel this default behavior by returning ER_HANDLED.
  if (ui::EventCanceledDefaultHandling(*event))
    return;

  // We also update the current window component here because for the
  // mouse-drag-release-press case, where the mouse is released and
  // pressed without mouse move event.
  int component =
      target->delegate()->GetNonClientComponent(event->location());
  if ((event->flags() &
        (ui::EF_IS_DOUBLE_CLICK | ui::EF_IS_TRIPLE_CLICK)) == 0 &&
      WindowResizer::GetBoundsChangeForWindowComponent(component)) {
    gfx::Point location_in_parent(
        ConvertPointToParent(target, event->location()));
    AttemptToStartDrag(target, location_in_parent, component,
                       aura::client::WINDOW_MOVE_SOURCE_MOUSE);
  } else {
    window_resizer_.reset();
  }
  if (WindowResizer::GetBoundsChangeForWindowComponent(component) != 0)
    event->StopPropagation();
}

void ToplevelWindowEventHandler::HandleMouseReleased(
    aura::Window* target,
    ui::MouseEvent* event) {
  if (event->phase() != ui::EP_PRETARGET)
    return;

  CompleteDrag(event->type() == ui::ET_MOUSE_RELEASED ?
                   DRAG_COMPLETE : DRAG_REVERT);
  // Completing the drag may result in hiding the window. If this happens
  // return true so no other handlers/observers see the event. Otherwise
  // they see the event on a hidden window.
  if (event->type() == ui::ET_MOUSE_CAPTURE_CHANGED &&
      !target->IsVisible()) {
    event->StopPropagation();
  }
}

void ToplevelWindowEventHandler::HandleDrag(
    aura::Window* target,
    ui::LocatedEvent* event) {
  // This function only be triggered to move window
  // by mouse drag or touch move event.
  DCHECK(event->type() == ui::ET_MOUSE_DRAGGED ||
         event->type() == ui::ET_TOUCH_MOVED ||
         event->type() == ui::ET_GESTURE_SCROLL_UPDATE);

  // Drag actions are performed pre-target handling to prevent spurious mouse
  // moves from the move/size operation from being sent to the target.
  if (event->phase() != ui::EP_PRETARGET)
    return;

  if (!window_resizer_)
    return;
  window_resizer_->resizer()->Drag(
      ConvertPointToParent(target, event->location()), event->flags());
  event->StopPropagation();
}

void ToplevelWindowEventHandler::HandleMouseMoved(
    aura::Window* target,
    ui::LocatedEvent* event) {
  // Shadow effects are applied after target handling. Note that we don't
  // respect ER_HANDLED here right now since we have not had a reason to allow
  // the target to cancel shadow rendering.
  if (event->phase() != ui::EP_POSTTARGET)
    return;

  // TODO(jamescook): Move the resize cursor update code into here from
  // CompoundEventFilter?
  internal::ResizeShadowController* controller =
      Shell::GetInstance()->resize_shadow_controller();
  if (controller) {
    if (event->flags() & ui::EF_IS_NON_CLIENT) {
      int component =
          target->delegate()->GetNonClientComponent(event->location());
      controller->ShowShadow(target, component);
    } else {
      controller->HideShadow(target);
    }
  }
}

void ToplevelWindowEventHandler::HandleMouseExited(
    aura::Window* target,
    ui::LocatedEvent* event) {
  // Shadow effects are applied after target handling. Note that we don't
  // respect ER_HANDLED here right now since we have not had a reason to allow
  // the target to cancel shadow rendering.
  if (event->phase() != ui::EP_POSTTARGET)
    return;

  internal::ResizeShadowController* controller =
      Shell::GetInstance()->resize_shadow_controller();
  if (controller)
    controller->HideShadow(target);
}

void ToplevelWindowEventHandler::SetWindowShowTypeFromGesture(
    aura::Window* window,
    wm::WindowShowType new_show_type) {
  wm::WindowState* window_state = ash::wm::GetWindowState(window);
  switch (new_show_type) {
    case wm::SHOW_TYPE_MINIMIZED:
      if (window_state->CanMinimize()) {
        window_state->Minimize();
        window_state->set_always_restores_to_restore_bounds(true);
        window_state->SetRestoreBoundsInParent(pre_drag_window_bounds_);
      }
      break;
    case wm::SHOW_TYPE_MAXIMIZED:
      if (window_state->CanMaximize()) {
        window_state->SetRestoreBoundsInParent(pre_drag_window_bounds_);
        window_state->Maximize();
      }
      break;
    case wm::SHOW_TYPE_LEFT_SNAPPED:
      if (window_state->CanSnap()) {
        window_state->SetRestoreBoundsInParent(pre_drag_window_bounds_);
        internal::SnapSizer::SnapWindow(window_state,
                                        internal::SnapSizer::LEFT_EDGE);
      }
      break;
    case wm::SHOW_TYPE_RIGHT_SNAPPED:
      if (window_state->CanSnap()) {
        window_state->SetRestoreBoundsInParent(pre_drag_window_bounds_);
        internal::SnapSizer::SnapWindow(window_state,
                                        internal::SnapSizer::RIGHT_EDGE);
      }
      break;
    default:
      NOTREACHED();
  }
}

void ToplevelWindowEventHandler::ResizerWindowDestroyed() {
  // We explicitly don't invoke RevertDrag() since that may do things to window.
  // Instead we destroy the resizer.
  window_resizer_.reset();

  CompleteDrag(DRAG_REVERT);
}

}  // namespace ash
