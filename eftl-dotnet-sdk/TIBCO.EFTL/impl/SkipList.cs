/*
 * Copyright (c) 2009-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: SkipList.cs 103512 2018-09-04 22:57:51Z bpeterse $
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.Threading;

namespace TIBCO.EFTL 
{
    internal class SkipList<T> 
    {
        private Object        _lock   = new Object();
        // fix the highest level to 33.
        private SkipListNode  _head   = new SkipListNode(0, default(T), 33);
        private Random        _rand   = new Random();
        private int           _levels = 1;
        private int           count   = 0;

        internal class SkipListNode 
        {
            internal SkipListNode[] Neighbors;
            internal long seqNum;
            internal T data;
            internal int numLevels;

            internal SkipListNode(long seqNum, T data, int level) 
            {
                this.seqNum = seqNum;
                this.data = data;
                this.Neighbors = new SkipListNode[level];
                this.numLevels = level;
            }
        }

        internal void Put(long seqNum, T data) 
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
                SkipListNode newNode = new SkipListNode(seqNum, data, level + 1);
                count++;
                SkipListNode current = _head;

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

        internal T Get(long seqNum) 
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
                            return current.Neighbors[i].data;
                    }
                }
                return default(T);
            }
        }

        internal bool Remove(long seqNum, out T returnValue) 
        {
            SkipListNode current = _head;
            bool found = false;
            returnValue = default(T);

            lock(_lock) 
            {
                for (int i = _levels - 1; i >= 0; i--) 
                {
                    for (; current.Neighbors[i] != null; current = current.Neighbors[i]) 
                    {
                        if (current.Neighbors[i].seqNum == seqNum) 
                        {
                            T data = current.Neighbors[i].data;

                            current.Neighbors[i] = current.Neighbors[i].Neighbors[i];
                            returnValue = data;
                            found = true;
                            count--;
                            break;
                        }

                        if (current.Neighbors[i].seqNum > seqNum) 
                        {
                            if (!found) 
                            {
                                found = false;
                                returnValue = default(T);
                                break;
                            }
                        }
                    }
                }
            }

            if (!found)
                returnValue = default(T);

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
                            T data = current.Neighbors[i].data;

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
                _head = new SkipListNode(0, default(T), 33);
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
                    T data = default(T);
                    Remove(uptoSeqnum, out data);

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

            while (current != null) 
            {
                list.Add(current.data);
                current = current.Neighbors[0];
            }
            return list;
        }
    }
}
