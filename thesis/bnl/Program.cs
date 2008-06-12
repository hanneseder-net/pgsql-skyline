using System;
using System.Collections.Generic;
using System.Text;

namespace bnl
{
    struct Tuple
    {
        public string id;
        public int d1;
        public int d2;
        public int ts;

        public Tuple(string id, int d1, int d2)
        {
            this.id = id;
            this.d1 = d1;
            this.d2 = d2;
            this.ts = 0;
        }

        public Tuple(string id)
        {
            this.id = id;
            d1 = 0;
            d2 = 0;
            ts = 0;
        }

        //public bool dominates(Tuple q)
        //{
        //    return d1 <= q.d1 && d2 <= q.d2 && (d1 < q.d1 || d2 < q.d2);
        //}

        public bool dominates(Tuple q)
        {
            return id.StartsWith("c") && q.id.StartsWith("a");
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            int windowsize = 1;
            int temptuples = 2;
            Queue<Tuple> I = CreateDataset(windowsize, temptuples);
            Queue<Tuple> O = new Queue<Tuple>();
            FixedBNL(windowsize, I, O);
            PipeOut(O);

            I = CreateDataset(windowsize, temptuples);
            O = new Queue<Tuple>();
            OriginalBNL(windowsize, I, O); // will not return 
            PipeOut(O);
        }

        private static Queue<Tuple> CreateDataset(int windowsize, int temptuples)
        {
            Queue<Tuple> I = new Queue<Tuple>();

            for (int i = 1; i <= windowsize; i++)
                I.Enqueue(new Tuple(String.Format("a{0}", i)));

            for (int i = 1; i <= temptuples; i++)
                I.Enqueue(new Tuple(String.Format("b{0}", i)));

            for (int i = 1; i <= windowsize; i++)
                I.Enqueue(new Tuple(String.Format("c{0}", i)));

            return I;
        }


        private static void FixedBNL(int windowsize, Queue<Tuple> I, Queue<Tuple> O)
        {
            List<Tuple> W = new List<Tuple>();
            Queue<Tuple> T = new Queue<Tuple>();
            
            int tsIn = 0;
            int tsOut = 0;
            
            while (true)
            {
                Propagate(W, O, tsIn);

                Tuple p;

                if (I.Count == 0)
                {
                    if (tsOut == 0)
                        break;

                    foreach (Tuple q in T)
                    {
                        I.Enqueue(q);
                    }
                    T.Clear();

                    tsIn = 0;
                    tsOut = 0;

                    continue;
                }

                p = I.Dequeue();
                tsIn++;
                p.ts = tsOut;
                bool iscandiate = true;
                
                dominancecheck:
                foreach (Tuple q in W)
                {
                    if (q.dominates(p))
                    {
                        iscandiate = false;
                        break;
                    }
                    else if (p.dominates(q))
                    {
                        W.Remove(q);
                        goto dominancecheck;
                    }
                }

                if (iscandiate)
                {
                    if (W.Count < windowsize)
                    {
                        W.Add(p);
                    }
                    else
                    {
                        T.Enqueue(p);
                        tsOut++;
                    }
                }

                
            }

            foreach (Tuple q in W)
            {
                O.Enqueue(q);
            }
        }


        private static void OriginalBNL(int windowsize, Queue<Tuple> I, Queue<Tuple> O)
        {
            List<Tuple> W = new List<Tuple>();
            Queue<Tuple> T = new Queue<Tuple>();

            int tsIn = 0;
            int tsOut = 0;

            while (I.Count > 0)
            {
                Propagate(W, O, tsIn);

                Tuple p = I.Dequeue();
                tsIn++;
                p.ts = tsOut;
                bool iscandiate = true;

            dominancecheck:
                foreach (Tuple q in W)
                {
                    if (q.dominates(p))
                    {
                        iscandiate = false;
                        break;
                    }
                    else if (p.dominates(q))
                    {
                        W.Remove(q);
                        goto dominancecheck;
                    }
                }

                if (iscandiate)
                {
                    if (W.Count < windowsize)
                    {
                        W.Add(p);
                    }
                    else
                    {
                        T.Enqueue(p);
                        tsOut++;
                    }
                }

                if (I.Count == 0)
                {
                    if (tsOut == 0)
                        break;

                    foreach (Tuple q in T)
                    {
                        I.Enqueue(q);
                    }
                    T.Clear();

                    tsIn = 0;
                    tsOut = 0;
                }
            }

            foreach (Tuple q in W)
            {
                O.Enqueue(q);
            }
        }

        private static void PipeOut(Queue<Tuple> O)
        {
            foreach (Tuple q in O)
            {
                Console.WriteLine(q.id);
            }
            Console.WriteLine();
        }
        private static void Propagate(List<Tuple> W, Queue<Tuple> O, int tsIn)
        {
            propagte:
            foreach (Tuple q in W)
            {
                if (q.ts == tsIn)
                {
                    O.Enqueue(q);
                    W.Remove(q);
                    goto propagte;
                }
            }
        }
    }
}
