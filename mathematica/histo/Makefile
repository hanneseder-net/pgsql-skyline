
datafiles=\
	i2d1e2.csv c2d1e2.csv a2d1e2.csv \
	i2d1e3.csv c2d1e3.csv a2d1e3.csv \
	i2d1e5.csv c2d1e5.csv a2d1e5.csv \
	i2d1e6.csv c2d1e6.csv a2d1e6.csv \
	c3d1e5.csv a3d1e5.csv

all: $(datafiles)

i2d1e2.csv:
	randdataset.exe -i -d 2 -n 100 > $@

c2d1e2.csv:
	randdataset.exe -c -d 2 -n 100 > $@

a2d1e2.csv:
	randdataset.exe -a -d 2 -n 100 > $@

i2d1e3.csv:
	randdataset.exe -i -d 2 -n 1000 > $@

c2d1e3.csv:
	randdataset.exe -c -d 2 -n 1000 > $@

a2d1e3.csv:
	randdataset.exe -a -d 2 -n 1000 > $@

i2d1e5.csv:
	randdataset.exe -i -d 2 -n 100000 -S > $@

c2d1e5.csv:
	randdataset.exe -c -d 2 -n 100000 -S > $@

a2d1e5.csv:
	randdataset.exe -a -d 2 -n 100000 -S > $@

i2d1e6.csv:
	randdataset.exe -i -d 2 -n 1000000 > $@

c2d1e6.csv:
	randdataset.exe -c -d 2 -n 1000000 > $@

a2d1e6.csv:
	randdataset.exe -a -d 2 -n 1000000 > $@

c3d1e5.csv:
	randdataset.exe -c -d 3 -n 100000 > $@

a3d1e5.csv:
	randdataset.exe -a -d 3 -n 100000 > $@

clean:
	rm -rf $(datafiles)
