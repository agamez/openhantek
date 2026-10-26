---
layout: default
---
## Valid specs for the 6022BE/BL with [custom firmware by Ho-Ro](https://github.com/Ho-Ro/Hantek6022API).

   - The device has no internal storage (except for a small fifo buffer), all data is streamed in real time as USB bulk transfer.
   - Supports 48, 30, 24, 16, 15, 12, 10, 8, 6, 5, 4, 3, 2, 1 MS/s and 500, 200, 100, 60 kS/s samplerates.
   - Due to the USB bandwith constraints the max usable samplerate is limited: Max 30 MS/s for CH1 only and max 15 MS/s for CH1+CH2 and also CH2 only - otherwise data overrun occurs.
   - Under Linux (even better if using a realtime kernel) the priority of the program can be raised when the user is member of a *realtime* group, e.g. audio:


    1. set limits in /etc/security/limits.d:
       @audio - rtprio 99
    2. add user to the group, e.g. audio:
       usermod -a -G audio <your_user_name>
    or set the limits only for your user in /etc/security/limits.d:
       <your_user_name> - rtprio 99

    - The scope program streams blocks of 2000 to 1000000 samples (default 1000000) - either 8 bit samples for CH1 only or 16 bit samples otherwise. This matches the documented 1 Mbyte per-channel memory depth. The *Capture depth* control selects the requested block size.
    - In Roll mode without *Record navigation* the free-running display uses the small internal roll buffer (about 10 kS), so the capture depth control does not change that buffer. Enable *Record navigation* to capture the selected depth in Roll mode as well.
    - With *Record navigation* enabled the complete captured block is retained and centered on the trigger. It can be navigated with the *Position* control. The arrow buttons move by half a visible window and *Go to trigger* restores the centered trigger view. The scope display always remains a normal ten-division window at the selected timebase; moving the control selects another window from the capture. For the triggered modes this is a display mode only: trigger search, trigger status and Single-shot stop behave exactly as with *Record navigation* disabled, and the samplerate is selected in the same way. Marker time readouts remain relative to the trigger, including when it lies outside the visible window. The top trigger marker remains visible and is pinned to the appropriate border when the trigger lies outside the window. In Roll mode *Record navigation* also sizes the free-running capture, as described above.
   - Two analog input channels with 8-bit sample-width, max input voltage range is -5 V..+5 V, values outside this range are clipped (shown as minimum or maximum value).
   - For low effective sampling rates < 1 MS/s a 10X..100X oversampling is used to increase the signal-to-noise ratio by 10..20 dB and to get a better voltage resolution (up to > 11 effective bits).
   - The first samples are unstable (due to the use of the ADC far outside the common mode voltage specifications) -> take about 2000 additional values to settle the ADC and drop the first samples.
   - No AC coupling available, all signals are measured as DC. (Use either a BNC DC-block or the math mode "CHx AC").
   - A [little HW modification](HANTEK6022_AC_Modification.pdf) adds AC coupling. OpenHantek6022 supports this feature since v2.17-rc5 / FW0204.
   - No HW trigger available, SW trigger seaches for trigger condition and positions the trace window accordingly.
   - Can detect rising or falling edge (also alternating from trace to trace) of the signal, displayed trigger position can be at 0..100% of screen width.
