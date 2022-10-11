/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 */

package com.tibco.ftl.samples;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.text.*;

/*
 * Tracks basic statistics.
 */
public class TibStat
{

    long n;

    long min_x;

    long max_x;

    double mean_x;

    long M2;

    double fctr; // 1.0 to get results in seconds

    double histRes; // 0.0 for no histogram

    private void init(double factor, double histogramResolution)
    {
        fctr = factor;
        histRes = histogramResolution;
    }

    TibStat()
    {
        init(1.0, 0.0);
    }

    TibStat(double resolution, double histogramResolution)
    {
        init(resolution, histogramResolution);
    }

    public void update(long x)
    {

        double delta;

        if(n == 0)
        {
            min_x = max_x = x;
        }
        else if(x < min_x)
        {
            min_x = x;
        }
        else if(x > max_x)
        {
            max_x = x;
        }

        n++;
        delta = (double) x - mean_x;
        mean_x += delta / (double) n;
        M2 += delta * (double) (x - mean_x);
    }

    double stdDeviation()
    {
        if(n != 0 && M2 >= 0.0)
            return Math.sqrt(M2 / (double) n);
        else
            return 0.0;
    }

    public void clear()
    {
    }

    public void accumulate(TibStat update)
    {
        long acc_n = n;
        long total_n = n + update.n;

        if(n == 0)
        {
            max_x = update.max_x;
            min_x = update.min_x;
        }
        else
        {
            if(max_x < update.max_x)
                max_x = update.max_x;
            if(min_x > update.min_x)
                min_x = update.min_x;
        }

        n = total_n;

        mean_x = (mean_x * acc_n + update.mean_x * update.n) / total_n;

        M2 += update.M2;
    }

    public double getCount()
    {
        return n;
    }

    public String toString()
    {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(baos);
        print(ps, 1.0);
        return baos.toString();

    }

    public String toTimerString()
    {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(baos);
        print(ps, timerScale());
        return baos.toString();

    }

    static void printEng(PrintStream out, double n)
    {

        if(n < 1)
        {
            int power;
            double factor;

            if(n == 0.0)
            {
                out.append("  0.00E+00");
                return;
            }

            power = (int) (Math.floor(Math.log10(n) / 3.0) * 3.0);
            factor = Math.exp(power * Math.log(10));

            out.printf("%3.2fE%+03d", n / factor, power);
        }
        else
        {
            out.print(NumberFormat.getNumberInstance().format(n));
        }
    }

    public void print(PrintStream stream, double factor)
    {
        stream.printf("min/max/avg/dev: ");
        printEng(stream, min_x * factor);
        stream.printf(" / ");
        printEng(stream, max_x * factor);
        stream.printf(" / ");
        printEng(stream, mean_x * factor);
        stream.printf(" / ");
        printEng(stream, stdDeviation() * factor);
    }

    public void printForRegressionTest(PrintStream stream, double factor)
    {
        stream.printf("%12g  ", n);
        stream.printf("%.12g, ", min_x * factor);
        stream.printf("%.12g, ", max_x * factor);
        stream.printf("%.12g, ", mean_x * factor);
        stream.printf("%.12g, ", stdDeviation() * factor);
        stream.printf("%.12g", M2 * factor);
    }

    static public long timerNow()
    {
        return System.nanoTime();
    }

    static public double timerScale()
    {
        return .000000001;
    }
}
