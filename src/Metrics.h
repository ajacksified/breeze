/* 
 * File:   Metrics.h
 * Author: szavadski
 *
 * Created on December 8, 2012, 10:44 AM
 */

#ifndef METRICS_H
#define	METRICS_H

#include "common.h"

class Metrics
{
public:
    Metrics();
    Metrics( const Metrics& orig );
    virtual ~Metrics( );
    void reset();
    void add( const Metrics& metrics, unsigned int time = 0 );
    
    unsigned int responseTime() const
    {
        return m_responseTime;
    }
    
    unsigned int requestCount() const
    {
        return m_requestCount;
    }
    
    unsigned int errorCount() const
    {
        return m_errorCount;
    }
    
    double averageResponseTime() const
    {
        return m_requestCount > 0 ? ( double ) m_responseTime / ( double ) m_requestCount : 0;     
    }
    
    double throughput()
    {
        return ( double ) m_requestCount / ( double )  m_time * 60;
    }
    
    double errorRate() const
    {
        return ( double ) m_errorCount /  ( double ) m_time * 60 ;
    }
    
    unsigned int time() const
    {
        return m_time;
    }
    
    void collect( unsigned int responseTime, bool error, const char* url );
    
private:
    unsigned int m_responseTime;
    unsigned int m_requestCount;
    unsigned int m_errorCount;
    
    //
    //  assume time is in seconds
    //
    unsigned int m_time;
};

#endif	/* METRICS_H */

