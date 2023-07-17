/*
// Copyright (c) 2018-2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include "chrometracer.h"
#include "intercept.h"

void CChromeTracer::init( const std::string& fileName )
{
    m_ProcessId = m_pIntercept->OS().GetProcessID();
    m_BufferSize = 1024;

    if( m_BufferSize != 0 )
    {
        fprintf(stderr, "Note: record size is %zu bytes.\n", sizeof(Record));
        m_RecordBuffer.reserve( m_BufferSize );
    }

    m_InterceptTrace.open(
        fileName.c_str(),
        std::ios::out | std::ios::binary );
    m_InterceptTrace << "[\n";
}

// Notes for the future:
// Printing into a pre-allocated string buffer and then writing is
// measured to be faster than calling fprintf or stream insertion
// operators.
// Handling each of these four cases separately eliminates the need
// to concatenate strings and reduces overhead.

// Call Logging
void CChromeTracer::writeCallLogging(
    const char* name,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s\""
        ",\"ts\":%" PRIu64 ",\"dur\":%" PRIu64 "},\n",
        m_ProcessId,
        threadId,
        name,
        startTime,
        delta );
    m_InterceptTrace.write(m_StringBuffer, size);
}

// Call Logging with Tag
void CChromeTracer::writeCallLogging(
    const char* name,
    const char* tag,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s( %s )\""
        ",\"ts\":%" PRIu64 ",\"dur\":%" PRIu64 "},\n",
        m_ProcessId,
        threadId,
        name,
        tag,
        startTime,
        delta );
    m_InterceptTrace.write(m_StringBuffer, size);
}

// Call Logging with Id
void CChromeTracer::writeCallLogging(
    const char* name,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta,
    uint64_t id )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s\""
        ",\"ts\":%" PRIu64 ",\"dur\":%" PRIu64 ",\"args\":{\"id\":%" PRIu64 "}},\n",
        m_ProcessId,
        threadId,
        name,
        startTime,
        delta,
        id );
    m_InterceptTrace.write(m_StringBuffer, size);

    if( m_pIntercept->config().ChromeFlowEvents )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"s\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"Command\""
            ",\"cat\":\"Commands\",\"ts\":%" PRIu64 ",\"id\":%" PRIu64 "},\n",
            m_ProcessId,
            threadId,
            startTime,
            id );
        m_InterceptTrace.write(m_StringBuffer, size);
    }
}

// Call Logging with Tag and Id
void CChromeTracer::writeCallLogging(
    const char* name,
    const char* tag,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta,
    uint64_t id )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s( %s )\""
        ",\"ts\":%" PRIu64 ",\"dur\":%" PRIu64 ",\"args\":{\"id\":%" PRIu64 "}},\n",
        m_ProcessId,
        threadId,
        name,
        tag,
        startTime,
        delta,
        id );
    m_InterceptTrace.write(m_StringBuffer, size);

    if( m_pIntercept->config().ChromeFlowEvents )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"s\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"Command\""
            ",\"cat\":\"Commands\",\"ts\":%" PRIu64 ",\"id\":%" PRIu64 "},\n",
            m_ProcessId,
            threadId,
            startTime,
            id );
        m_InterceptTrace.write(m_StringBuffer, size);
    }
}

void CChromeTracer::flushRecords()
{
    CLIntercept::clock::time_point  start, end;
    size_t count = m_RecordBuffer.size();
    start = CLIntercept::clock::now();

    for( const auto& rec : m_RecordBuffer )
    {
        switch( rec.Type )
        {
        case RecordType::CallLogging:
            writeCallLogging(
                rec.Name.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta );
            break;
        case RecordType::CallLoggingTag:
            writeCallLogging(
                rec.Name.c_str(),
                rec.Tag.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta );
            break;
        case RecordType::CallLoggingId:
            writeCallLogging(
                rec.Name.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta,
                rec.CallLogging.Id );
            break;
        case RecordType::CallLoggingTagId:
            writeCallLogging(
                rec.Name.c_str(),
                rec.Tag.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta,
                rec.CallLogging.Id );
            break;

        case RecordType::DeviceTiming:
        case RecordType::DeviceTimingPerKernel:
        case RecordType::DeviceTimingInStages:
        case RecordType::DeviceTimingInStagesPerKernel:
        default: CLI_ASSERT(0); break;
        }
    }

    m_RecordBuffer.clear();

    end = CLIntercept::clock::now();
    using us = std::chrono::microseconds;
    uint64_t    usDelta = std::chrono::duration_cast<us>(end - start).count();
    fprintf(stderr, "Wrote %zu chrome tracing records in %" PRIu64 "us.\n", count, usDelta);
}
