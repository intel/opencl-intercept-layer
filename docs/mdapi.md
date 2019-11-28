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
sampling, similar to the way an OpenCL command queue is supported that supports
event profiling.  Commands that are enqueued into this command queue support a
new `clGetEventProfilingInfo` query to return a buffer containing MDAPI performance
counter deltas.  This buffer can then be passed to MDAPI to decode and log the
MDAPI performance counters for each event.

MDAPI event-based sampling has been supported for some time and is the most robust
mechanism to collect MDAPI performance metrics, however it has some limitations:

* MDAPI event-based sampling is currently only available on Windows.
* MDAPI event-based sampling on Linux is tracked here: [intel/compute-runtime #182](https://github.com/intel/compute-runtime/issues/182).
* MDAPI event-based sampling is unlikely to be supported on OSX.

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

MDAPI time-based sampling does not rely on any functionality in the OpenCL
implementation itself and hence may be supported on wherever MDAPI is supported.
MDAPI time-based sampling has been tested on Windows and Linux.  MDAPI time-based
sampling may eventually be supported on OSX (see [intel/opencl-intercept-layer #105](https://github.com/intel/opencl-intercept-layer/issues/105))
but is not currently implemented or tested.

MDAPI time-based sampling is not as precise as MDAPI event-based sampling, but
because MDAPI time-based sampling does not rely on event profiling, MDAPI
time-based sampling does not serialize command execution and can measure
and profile concurrent execution via out-of-order command queues.

### How to Enable MDAPI Time-Based Sampling

To enable MDAPI Time-Based Sampling, set the following controls:

* **DevicePerfCounterCustom**: Specify the metrics to collect, typically "ComputeBasic".
* **DevicePerfCounterTimeBasedSampling**: Set to `1` (enabled).

These controls can be enabled via `cliloader`, by specifying the `--mdapi-tbs` option.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2019, Intel(R) Corporation
