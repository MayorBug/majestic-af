# HiEasy P035 Focus Behavior

These observations apply to the HiEasy P035 camera with a p6slite controller.
They do not describe the H07.

## Controller behavior

When focus matching is enabled, the controller keeps the image close to focus during zoom movement.

After zoom movement, the controller moves the focus back and forth near the focal point. This motion is called focus breathing in this note.

The controller appears to know an approximate focus range for each zoom
position. A future UART capture can test this assumption.

A standard Pelco D STOP command does not stop the focus breathing.

The P035 can miss one stop frame after a commanded movement. The motor driver
sends three stop frames with 2 ms gaps for this controller.

A short manual focus pulse stops the breathing. A pulse of approximately 70 ms in either direction works.

After that pulse, small manual pulses can move the lens to the exact focal point.

The controller can have a vendor command that stops the breathing. This command is not known.

One stock camera is available for a future UART capture of the vendor protocol.

## Why the original AF2 path failed

The motor protocol worked, but the AF2 position model did not match this
controller.

The P035 does not report focus position, movement time, or zoom magnification.
Its focus matching can also move the lens without an AF request. These traits
invalidate AF2's saved position and calibrated zoom-to-focus model.

## Blind-seek result

The controller focus matching remains useful because it leaves the lens near
focus after zoom movement.

Blind seek uses only the live ISP focus metric. It estimates position only
during one pass and never seeks an endpoint.

The [blind-seek autofocus document](blind-seek-autofocus.md) describes the search sequence and timing.

The focus motor starts in one direction. The ISP focus metric then tells the
search when to continue, reverse, and return to the best observed value.

The correction stage waits 300 ms after each focus pulse. Shorter waits exposed
stale focus measurements during camera tests.

Camera tests found that blind seek can return near the observed metric peak
from either focus direction. The search remains experimental and needs tests
across more scenes, zoom levels, and light levels.

A stock camera remains available for a future UART capture. That capture can
look for a direct controller command that ends focus breathing.
