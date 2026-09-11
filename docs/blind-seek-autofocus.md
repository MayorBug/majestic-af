# Blind-Seek Autofocus

`af_blind_seek` has no knowledge of lens position. It issues relative focus
movements and observes only the ISP focus metric. It does not use motor
feedback, position tracking, zoom magnification, or a calibrated zoom-to-focus
model. The P035 uses this algorithm.

Select this path in `majestic.yaml`:

```yaml
isp:
  autofocus:
    enabled: true
    algorithm: blind_seek
```

The plugin reads the algorithm when AF first becomes available. It keeps this
choice until Majestic reloads the plugin.

The path starts near focus because the P035 controller does focus matching
during zoom movement. It never seeks a focus endpoint.

## Why the P035 does not use AF2

The P035 autofocus board does not report the focus position. It also does not
report when the focus motor starts or finishes a movement.

The board gives no timing feedback for a focus pulse. The software knows only
the requested direction and pulse length. These values do not prove how far
the lens moved.

The board can move focus independently while it matches focus during zoom.
This movement makes a saved software position invalid.

AF2 uses a time-based position model, measured travel, and a known zoom
magnification. It can find a position reference by driving to an endpoint.

The P035 interface supplies no position or timing feedback. Its automatic
focus matching can also invalidate the time-based position model.

The blind-seek model uses the ISP focus metric as its feedback. It keeps a relative
position estimate only during one search and discards the estimate afterward.

The AF model requests timed focus pulses from `motorsd`. The selected driver
controls each pulse duration and reports when it completes the request.

This timing removes client scheduling delay from the pulse length. It does not
prove the physical lens movement because the P035 supplies no motor feedback.

## Start timing

An autofocus request with `settle` waits 1200 ms after the zoom sequence.

The engine then waits 150 ms before it reads the first focus value. The total
quiet time after zoom is 1350 ms.

The 150 ms delay lets the controller finish its focus matching. It is not the
delay after a focus pulse.

## Manual control

A manual motor client has priority over AF. `motorsd` ends the AF lease before
it gives control to the manual client.

The AF adapter subscribes to lease events. The worker cancels its current AF
job after it receives a preemption event.

## Search sequence

1. Read the initial focus value.
2. Start continuous movement in one direction.
3. Read the focus value while the lens moves.
4. Reverse after two readings show a decrease of at least 2 percent.
5. After a reversal, wait for two rising readings.
6. Track the best value in the current sweep independently of the historic peak.
7. If the value does not change by 2 percent in five seconds, return for five seconds.
8. Search the other direction after the return.
9. Record the best focus value and its estimated position.
10. Continue through the peak until two readings confirm the decrease.
11. Return to the best recorded position.
12. Use correction pulses if gear slack prevents an accurate return.
13. Reduce the correction step after a confirmed overshoot.
14. Wait for settling and verify the result before reporting success.

The first direction is arbitrary because the P035 gives no position data. The
focus metric decides whether the search continues or reverses.

The recovery sweep arms after two rising steps establish a 2 percent gain from
its lowest reading. Equal readings preserve the rising trend. Two readings
at least 2 percent below this sweep's peak stop recovery. The historic peak
remains the final quality target; being below it does not mean focus is falling.
A normal sweep permits a 12 percent decrease.

The peak error selects each correction pulse:

- within 3 percent: 40 ms
- otherwise: 70 ms

The estimated return can land on either side of the peak. If a correction
reduces the metric by 2 percent, AF can change direction. Several 70 ms pulses
can clear reversal slack. AF does not use one large reversal pulse.

Correction pulses cannot exceed 70 ms. AF can change correction direction at
most twice. After improvement followed by a decline, AF halves the maximum
step, down to 20 ms. It permits at most 32 correction pulses within the total
budget.

The engine reads the live metric at short intervals during the continuous sweep.
It waits 300 ms after a correction pulse so the metric can follow the lens.

Five focus values form each measurement. The median value rejects short image
disturbances.

## Limits

The complete search has a 30-second budget. One continuous sweep can run for at
most 12 seconds. A new zoom request cancels the active search.

Success requires two settled measurements within 2 percent of the best value
observed during the pass. Otherwise the result reports incomplete convergence.

The return stage continues its correction while the focus value is more than
2 percent below the recorded peak.

The engine reports this path as path 3. It does not keep a focus position
between blind-seek searches.

Select `algorithm: af2` only for a calibrated lens with valid zoom
magnification. The plugin does not switch algorithms during an AF pass.

See [Focus Characterization](focus-characterization.md) for the repeatable lens
response capture procedure.

With `/tmp/af_trace.on` present, `/tmp/af_trace.csv` includes phase, sample start
and end timestamps, and the last requested direction and pulse length. The
`pos` column remains an estimate. A zero pulse length denotes continuous
movement; command fields do not prove physical motion during the measurement.

See [Future AF Research](future-af-research.md) for experiments that are not
part of the current algorithm.
