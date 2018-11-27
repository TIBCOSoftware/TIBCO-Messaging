/*
 * Copyright (c) 2011-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */
package com.tibco.ftl.samples;

import java.text.*;

public class TibAuxStatRecord
{
    int N;
    long max_x;
    long min_x;
    double mean_x;
    double M2;
    NumberFormat nf = new DecimalFormat(" ##0.00E00 ");

    public TibAuxStatRecord()
    {
        N = 0;
    }
    
    public void setN(int n)
    {
        this.N = n;
    }

    public void StatUpdate(long x)
    {
        double delta;

        if (N == 0)
        {
            min_x = max_x = x;
            mean_x = M2 = 0.0;
        } else if (x < min_x)
        {
            min_x = x;
        } else if (x > max_x)
        {
            max_x = x;
        }

        N++;
        delta = (double) x - mean_x;
        mean_x += delta / (double) N;
        M2 += delta * ((double) x - mean_x);
    }
    
    public String toString(double factor)
    {
        StringBuilder builder = new StringBuilder();

        builder.append("min/max/avg/dev: ");
        builder.append(nf.format(min_x * factor));
        builder.append("/");
        builder.append(nf.format(max_x * factor));
        builder.append("/");
        builder.append( nf.format(mean_x * factor));
        builder.append("/");
        builder.append(nf.format(StdDeviation() * factor));
        
        return builder.toString();
    }

    public void PrintStats(double factor)
    {
        System.out.printf("min/max/avg/dev: ");
        System.out.printf("%s/", nf.format(min_x * factor));
        System.out.printf("%s/", nf.format(max_x * factor));
        System.out.printf("%s/", nf.format(mean_x * factor));
        System.out.printf("%s", nf.format(StdDeviation() * factor));
    }

    public double StdDeviation()
    {
        if (N > 0 && M2 >= 0.0)
        {
            return Math.sqrt(M2 / (double) N);
        } else
        {
            return 0.0;
        }
    }
}
