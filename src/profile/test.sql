--<comment>

--</comment>

--<query runid=bar>
explain analyze select * from i15d1e3s0;
--</query>


--<query runid=bar2>
explain analyze select * from i15d1e3s0;
--</query>

--<query runid=bar3>
explain analyze select * from i15d1e3s0;
--</query>

--<query runid=foobar>
explain analyze select * from i15d1e3s0 skyline of d1 min, d2 min;
--</query>

--<query runid=foobar2>
explain analyze select * from i15d1e3s0 skyline of d1 min, d2 min;
--</query>

--<query runid=foobar3>
explain analyze select * from i15d1e3s0 skyline of d1 min, d2 min;
--</query>


--<query runid=foobar4>
explain analyze select * from i15d1e3s0 skyline of d1 min, d2 min;
--</query>

