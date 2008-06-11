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
            int windowsize = 3;
            int temptuples = 4;
            Queue<Tuple> I = new Queue<Tuple>();


            for (int i = 1; i <= windowsize; i++)
                I.Enqueue(new Tuple(String.Format("a{0}", i)));

            for (int i = 1; i <= temptuples; i++)
                I.Enqueue(new Tuple(String.Format("b{0}", i)));

            for (int i = 1; i <= windowsize; i++)
                I.Enqueue(new Tuple(String.Format("c{0}", i)));

            //I.Enqueue(new Tuple("b1", 2, 10));
            //I.Enqueue(new Tuple("b2", 4, 8));
            //I.Enqueue(new Tuple("c", 10, 1));
            //I.Enqueue(new Tuple("d1", 1, 9));
            //I.Enqueue(new Tuple("d2", 3, 7));

            List<Tuple> W = new List<Tuple>();
            Queue<Tuple> T = new Queue<Tuple>();
            Queue<Tuple> O = new Queue<Tuple>();

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
                    foreach (Tuple q in T)
                    {
                        I.Enqueue(q);
                    }
                    T.Clear();

                    Propagate(W, O, tsIn);

                    tsIn = 0;
                    tsOut = 0;
                }
            }

            foreach (Tuple q in W)
            {
                O.Enqueue(q);
            }

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
