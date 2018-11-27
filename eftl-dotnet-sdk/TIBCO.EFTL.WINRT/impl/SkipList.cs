/*
 * Copyright (c) 2009-$Date: 2016-03-15 18:26:14 -0500 (Tue, 15 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: SkipList.cs 84744 2016-03-15 23:26:14Z bpeterse $
 */
using System;

namespace TIBCO.EFTL
{
    public class SkipList
    {
        private Object        _lock   = new Object();
        // fix the highest level to 33.
        private SkipListNode  _head   = new SkipListNode(0, null, 33);
        private Random        _rand   = new Random();
        private int           _levels = 1;
        private int           count   = 0;

        internal void Put(long seqNum, PublishContext ctx)
        {
            lock(_lock)
            {
                int level = 0;
                int random = _rand.Next();
    
                for (int R = random; (R & 1) == 1; R >>= 1)
                {
                    level++;
                    if (level == _levels) 
                    {
                        _levels++; 
                        break; 
                    }
                }
                
                // Insert this node into the skip list
                SkipListNode newNode = new SkipListNode(seqNum, ctx, level + 1);
                count++;
                SkipListNode current =  _head;
    
                for (int i = _levels - 1; i >= 0; i--)
                {
                    for (; current.Neighbors[i] != null; current = current.Neighbors[i])
                    {
                        if (current.Neighbors[i].seqNum > seqNum) 
                            break;
                    }
                    
                    if (i <= level) 
                    { 
                        newNode.Neighbors[i] = current.Neighbors[i]; 
                        current.Neighbors[i] = newNode; 
                    }
                }
            }
        }

        internal PublishContext Get(long seqNum)
        {
            SkipListNode current = _head;

            lock(_lock)
            {
                for (int i = _levels - 1; i >= 0; i--)
                {
                    for (; current.Neighbors[i] != null; current = current.Neighbors[i])
                    {
                        if (current.Neighbors[i].seqNum > seqNum)
                            break;
    
                        if (current.Neighbors[i].seqNum == seqNum) 
                            return current.Neighbors[i].ctx;
                    }
                }
                return null;
            }
        }

        internal bool Remove(long seqNum, out PublishContext returnValue)
        {
            SkipListNode current = _head;
            bool found = false;
            returnValue = null;

            lock(_lock)
            {
                for (int i = _levels - 1; i >= 0; i--)
                {
                    for (; current.Neighbors[i] != null; current = current.Neighbors[i])
                    {
                        if (current.Neighbors[i].seqNum == seqNum) 
                        {
                            PublishContext ctx = current.Neighbors[i].ctx;

                            current.Neighbors[i] = current.Neighbors[i].Neighbors[i];
                            returnValue = ctx;
                            found = true;
                            count--;
                            break;
                        }

                        if (current.Neighbors[i].seqNum > seqNum)
                        {
                            if (!found)
                            {
                                found = false;
                                returnValue = null;
                                break;
                            }
                        }
                    }
                }
            }

            if (!found)
                returnValue = null;

            return found;
        }

        internal bool Remove(long seqNum)
        {
            SkipListNode current = _head;
            bool found = false;

            lock(_lock)
            {
                for (int i = _levels - 1; i >= 0; i--)
                {
                    for (; current.Neighbors[i] != null; current = current.Neighbors[i])
                    {
                        if (current.Neighbors[i].seqNum == seqNum) 
                        {
                            PublishContext ctx = current.Neighbors[i].ctx;

                            current.Neighbors[i] = current.Neighbors[i].Neighbors[i];
                            found = true;
                            count--;
                            break;
                        }

                        if (current.Neighbors[i].seqNum > seqNum)
                        {
                            if (!found)
                            {
                                found = false;
                                break;
                            }
                        }
                    }
                }
            }
            return found;
        }

        internal void Clear()
        {
            lock(_lock)
            {
                _head = new SkipListNode(0, null, 33);
                _levels = 1;
            }
        }

        internal void Clear(long seqNum)
        {
            // TODO: fix this function.
            long uptoSeqnum = seqNum;

            lock(_lock)
            {
                while (uptoSeqnum > 0)
                {
                    PublishContext ctx = null;
                    Remove(uptoSeqnum, out ctx);
                    
                    uptoSeqnum--;
                }
                return;
            }
        }

        internal bool Contains(long seqNum)
        {
            return false;
        }

        internal ArrayList Values()
        {
            ArrayList list = new ArrayList();
            SkipListNode current = _head.Neighbors[0];

            while(current != null)
            {
                list.Add(current.ctx);
                current = current.Neighbors[0];
            }
            return list;
        }
    }

    internal class SkipListNode
    {
        internal SkipListNode[] Neighbors;
        internal long           seqNum;
        internal PublishContext ctx;
        internal int            numLevels;
        
        internal SkipListNode(long seqNum, PublishContext ctx, int level)
        {
            this.seqNum      = seqNum;
            this.ctx         = ctx;
            this.Neighbors   = new SkipListNode[level];
            this.numLevels   = level;
        }
    }
}
