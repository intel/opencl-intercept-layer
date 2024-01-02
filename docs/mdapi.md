# Intel GPU MDAPI Performance Metrics

This document describes how to use the Intercept Layer for OpenCL Applications
to collect and aggregate low-level performance metrics for Intel GPU OpenCL
devices using the [Intel Metrics Discovery API](https://github.com/intel/metrics-discovery),
also known as MDAPI.

MDAPI supports two methods of metrics collection: event-based sampling, where
metrics are collected before and after a specific event, and time-based sampling
where metrics are collected at regular time intervals.

Both sampling methods are supported by the Intercept Layer for OpenCL Applications,
however support for MDAPI time-based sampling should be considered _experimental_.
MDAPI time-based sampling results will be collected and logged to a CSV file,
however additional post-processing will be required to analyze or visualize the
data.

## MDAPI Event-Based Sampling

MDAPI event-based sampling is implemented as an extension to standard OpenCL
[event profiling](https://www.khronos.org/registry/OpenCL/specs/2.2/html/OpenCL_API.html#event-profiling-info-table).
Effectively, an OpenCL command queue must be created that supports MDAPI event-based
sampling, similar to the way an OpenCL command queue is created that supports
event profiling.  Commands that are enqueued into this command queue support a
new `clGetEventProfilingInfo` query to return a buffer containing MDAPI performance
counter deltas.  This buffer can then be passed to MDAPI to decode and log the
MDAPI performance counters for each event.

MDAPI event-based sampling has been supported for some time and is the most robust
mechanism to collect MDAPI performance metrics, however it has some limitations:

* MDAPI event-based sampling is only available on Windows and newer Linux drivers.
* MDAPI event-based sampling is unlikely to be supported on OSX.
* The API to create an OpenCL command queue that supports MDAPI event-based
sampling currently does not support newer OpenCL command queue properties such
as command queue [priority hints](https://www.khronos.org/registry/OpenCL/specs/2.2/html/OpenCL_Ext.html#cl_khr_priority_hints)
and [throttle hints](https://www.khronos.org/registry/OpenCL/specs/2.2/html/OpenCL_Ext.html#cl_khr_throttle_hints).
OpenCL command queues that are created with these properties will not support
MDAPI event-based sampling.

Additionally, because MDAPI event-based sampling relies on an extension to event
profiling, event-based sampling may serialize all command execution, complicating
performance analysis when commands would otherwise execute concurrently, such as in
an out-of-order command queue.

### How to Enable MDAPI Event-Based Sampling

To enable MDAPI Event-Based Sampling, set the following controls:

* **DevicePerfCounterCustom**: Specify the metrics to collect, typically "ComputeBasic".
* **DevicePerfCounterEventBasedSampling**: Set to `1` (enabled).
* **DevicePerfCounterTiming**: Optionally, set to `1` (enabled) to include aggregated metrics in the report.

These controls can be enabled via `cliloader`, by specifying the `--mdapi-ebs` option.

## MDAPI Time-Based Sampling

MDAPI time-based sampling is implemented entirely within MDAPI itself, and relies
on internal MDAPI instrumentation and buffers to collect and report performance metrics
at regular time intervals.

Because MDAPI time-based sampling does not rely on any functionality in the OpenCL
implementation itself, it is supported on wherever MDAPI is supported.
MDAPI time-based sampling has been tested on Windows, Linux, and OSX.

MDAPI time-based sampling is not as precise as MDAPI event-based sampling, but
because MDAPI time-based sampling does not rely on event profiling, MDAPI
time-based sampling does not serialize command execution and can measure
and profile concurrent execution via out-of-order command queues.

### How to Enable MDAPI Time-Based Sampling

To enable MDAPI Time-Based Sampling, set the following controls:

* **DevicePerfCounterCustom**: Specify the metrics to collect, typically "ComputeBasic".
* **DevicePerfCounterTimeBasedSampling**: Set to `1` (enabled).

These controls can be enabled via `cliloader`, by specifying the `--mdapi-tbs` option.

## Notes and Tips

* On Windows, the MDAPI library is distributed with the GPU driver.
* On Linux, the MDAPI library should be built and installed from source.

    * https://github.com/intel/metrics-discovery

* Linux may also requires the "metrics library".  If required, it should also be
  built and installed from source.

    * https://github.com/intel/metrics-library

* On OSX, the path to the MDAPI library should be set manually with
`DevicePerfCounterLibName` control. The library is named `libigdmd.dylib` and
it usually resides under `/System/Library/Extensions/AppleIntel<CPU NAME>GraphicsMTLDriver.bundle/Contents/MacOS/libigdmd.dylib`,
where `<CPU NAME>` is a short name of your CPU generation. For example, on Kaby
Lake machines `<CPU NAME>` is `KBL`. You can also add path to `libigdmd.dylib`
library to `DYLD_LIBRARY_PATH` environment library, so that it can be found system-wide.
* Collecting MDAPI metrics currently requires elevated privileges
because metrics are collected system-wide.
* On Linux, MDAPI metrics may be enabled for non-root users
by setting `/proc/sys/dev/i915/perf_stream_paranoid` to `0`:

    ```sh
    $ echo 0 > /proc/sys/dev/i915/perf_stream_paranoid
    ```

    or:

    ```sh
    $ sysctl dev.i915.perf_stream_paranoid=0
    ```

    For more information, see:
    * https://software.intel.com/en-us/vtune-cookbook-real-time-monitoring-with-system-analyzer
* MDAPI metrics are logged to CSV files in the usual log directory.
* To debug MDAPI issues, consider enabling MDAPI logging by defining `MD_DEBUG` in
[MetricsDiscoveryHelper.cpp](../intercept/mdapi/MetricsDiscoveryHelper.cpp).

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
