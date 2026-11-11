# Proposed PR: Add DS4 touchpad mouse and original Steam Controller BLE support

Local PR description draft; no GitHub pull request has been opened.
Target: Anime0t4ku/MC-BlueBridge main. The owner authorized branch backup to their
own fork; hardware testing is still required before submitting this description.

## Problem and behavior

DS4 touchpad input is not exposed as mouse input, and the original Steam Controller
needs its dedicated BLE service rather than the generic HID path. This change adds
optional relative USB mouse output in single-controller MiSTer mode and a dedicated
original Steam Controller BLE client/parser. Ordinary controls continue through the
existing gamepad path. With mouse disabled, the Steam right pad is a right stick.

The Steam controller also has optional gyro-to-mouse aiming, sensitivity/inversion,
activation choices, hold-to-pause and right-trigger mouse fire. Defaults are off.
A controller-only Web Serial page and protocol commands save these options and
apply them after restart. The configuration extension migrates upstream settings.

## Scope

Controller support, required USB mouse/configuration plumbing, diagnostics, tests
and setup documentation only. No OLED, PEW, rotary encoder, trackball or physical
panel features. No unrelated latency/turbo/queue rewrite. A missing state include
and large management scratch allocations are corrected where needed by this path.

## Validation

Native DS4/Steam transport/parser/mouse/settings tests and simulated-serial tests
of the actual config page passed. Standalone build, descriptor audit and scope
review are recorded in CONTROLLER_TEST_PLAN.md. Hardware validation is pending;
do not present this draft as tested compatibility, latency or light-gun accuracy.

Before submission, replace this paragraph with actual DS4/Steam firmware, MiSTer
core and hardware test results. Review limitations and the settings migration
notes in CONTROLLERS.md and CONTROLLER_TEST_PLAN.md.
