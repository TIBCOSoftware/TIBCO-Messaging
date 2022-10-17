/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

package com.tibco.ftl.samples;

public class TibAux
{
    String[]    savedArgs       = null;
    String      usage           = null;
    
    public TibAux(
        String[]        args,
        String          u)
    {
        savedArgs = args;
        usage     = u;
    }

    void printUsage()
    {
        System.out.println(usage);
        System.exit(0);
    }

    public boolean getFlag(
        int             i,
        String          name,
        String          sname)
    {
        if (savedArgs[i].compareTo(name)==0 || savedArgs[i].compareTo(sname)==0)
            return true;

        return false;
    }

    public String getString(
        int             i,
        String          name,
        String          sname)
    {
        if (getFlag(i, name, sname))
        {
            if ((i+1) < savedArgs.length)
            {
                return savedArgs[i+1];
            }
            else
            {
                System.out.println("missing value for " + savedArgs[i]);
                printUsage();
            }
        }

        return null;
    }

    public int getInt(
        int             i,
        String          name,
        String          sname)
    {
        String          strVal  = null;
        int             val     = -1;

        strVal = getString(i, name, sname);
        if (strVal != null)
        {
            try 
            {
                val = Integer.parseInt(strVal);
            }
            catch(NumberFormatException e)
            {
                System.err.println("invalid value for " + savedArgs[i]);
                printUsage();
            }
        }

        return val;
    }

    public double getDouble(
        int             i,
        String          name,
        String          sname)
    {
        String          strVal  = null;
        double          val     = -1.0;

        strVal = getString(i, name, sname);
        if (strVal != null)
        {
            try 
            {
                val = Double.parseDouble(strVal);
            }
            catch(NumberFormatException e)
            {
                System.err.println("invalid value for " + savedArgs[i]);
                printUsage();
            }
        }

        return val;
    }
}
