/* 
 * File:   Metrics.cpp
 * Author: szavadski
 * 
 * Created on December 8, 2012, 10:44 AM
 */

#include "Metrics.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define STATS_REFRESH_TIMEOUT 300

Metrics::Metrics( )
{
    reset();
}

Metrics::~Metrics( )
{
}

void Metrics::reset()
{
    m_responseTime = 0;
    m_requestCount = 0;
    m_time = 0;
    m_errorCount = 0;
}

void Metrics::add( const Metrics& metrics, unsigned int time )
{
    if ( time )
    {
        m_time += time;
    }
    
    m_responseTime += metrics.responseTime();
    m_requestCount += metrics.requestCount();
    m_errorCount += metrics.errorCount();
    
    if ( m_time > STATS_REFRESH_TIMEOUT )
    {
        m_requestCount = ( unsigned int ) throughput() / 60 * time;
        m_responseTime = ( unsigned int ) averageResponseTime();
        m_errorCount = ( unsigned int ) errorRate() / 60 * time;
        
        m_time = time;
        
    }
    
    TRACE(
        "responseTime %d, requestCount %d, errorCount %d, time %d, avg response time %2.2f, throughput %2.2f, error rate %2.2f", 
         m_responseTime, m_requestCount, m_errorCount, m_time, averageResponseTime(), throughput(), errorRate()
    );

}

void Metrics::collect( unsigned int responseTime, bool error, const char* url )
{
    TRACE_ENTERLEAVE();
    TRACE("responseTime %d, error %d, url %s", responseTime, error, url );
    
    m_responseTime += responseTime;
    m_requestCount += 1;
    
    if ( error )
    {
        m_errorCount += 1;
    }
}