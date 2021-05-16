// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_INPUT_HINT_CHECKER_H_
#define BASE_ANDROID_INPUT_HINT_CHECKER_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/base_export.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"

namespace base::android {

BASE_DECLARE_FEATURE(kYieldWithInputHint);

// A class to track a single global root View object and ask it for presence of
// new unhandled input events.
//
// This class uses bits specific to Android V and does nothing on earlier
// releases.
//
// Must be constructed on UI thread. All public methods must be called on the UI
// thread.
class BASE_EXPORT InputHintChecker {
 public:
  InputHintChecker();
  virtual ~InputHintChecker();

  // Returns the singleton.
  static InputHintChecker& GetInstance();

  // Initializes features for this class. See `base::features::Init()`.
  static void InitializeFeatures();

  // Obtains a weak reference to |root_view| so that the following calls to
  // HasInput() take the input hint for this View. Requirements for the View
  // object are described in InputHintChecker.java.
  void SetView(JNIEnv* env, const jni_zero::JavaParamRef<jobject>& root_view);

  // Fetches and returns the input hint from the Android Framework.
  //
  // Works as a hint: when unhandled input events are detected, this method
  // returns |true| with high probability. However, the returned value neither
  // guarantees presence nor absence of input events in the queue. For example,
  // this method returns |false| while the singleton is going through
  // initialization.
  //
  // Throttles the calls to one every few milliseconds. When a call is made
  // before the minimal time interval passed since the previous call, returns
  // false.
  static bool HasInput() { return false; }

  // RAII override of GetInstance() for testing.
  struct ScopedOverrideInstance {
    explicit ScopedOverrideInstance(InputHintChecker* checker);
    ~ScopedOverrideInstance();
  };

  bool IsInitializedForTesting();
  bool FailedToInitializeForTesting();
  bool HasInputImplNoThrottlingForTesting(_JNIEnv* env);
  bool HasInputImplWithThrottlingForTesting(_JNIEnv* env);

 protected:
  virtual bool HasInputImplWithThrottling();

 private:
  friend class base::NoDestructor<InputHintChecker>;
  class OffThreadInitInvoker;
  enum class InitState;
  InitState FetchState() const;
  void TransitionToState(InitState new_state);
  void RunOffThreadInitialization();
  void InitGlobalRefsAndMethodIds(JNIEnv* env);
  bool HasInputImpl(JNIEnv* env, jobject o);

  base::TimeTicks last_checked_;

  // Initialization state. It is made atomic because part of the initialization
  // happens on another thread while public methods of this class can be called
  // on the UI thread.
  std::atomic<InitState> init_state_;

  // The android.view.View object reference used to fetch the hint in
  // HasInput().
  JavaObjectWeakGlobalRef view_;

  // Represents a reference to android.view.View.class. Used during
  // initialization.
  ScopedJavaGlobalRef<jobject> view_class_;

  // Represents a reference to object of type j.l.reflect.Method for
  // View#probablyHasInput().
  ScopedJavaGlobalRef<jobject> reflect_method_for_has_input_;

  // The ID corresponding to j.l.reflect.Method#invoke(Object, Object...).
  jmethodID invoke_id_;

  // The ID corresponding to j.l.Boolean#booleanValue().
  jmethodID boolean_value_id_;
  THREAD_CHECKER(thread_checker_);
};

}  // namespace base::android

#endif  // BASE_ANDROID_INPUT_HINT_CHECKER_H_
