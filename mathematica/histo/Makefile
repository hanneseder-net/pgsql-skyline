
datafiles=i2d1e5.csv c2d1e5.csv a2d1e5.csv i2d1e6.csv c2d1e6.csv a2d1e6.csv i2d1e3.csv c2d1e3.csv a2d1e3.csv

all: $(datafiles)

i2d1e3.csv:
	randdataset.exe -i -d 2 -n 1000 > $@

c2d1e3.csv:
	randdataset.exe -c -d 2 -n 1000 > $@

a2d1e3.csv:
	randdataset.exe -a -d 2 -n 1000 > $@

i2d1e5.csv:
	randdataset.exe -i -d 2 -n 100000 > $@

c2d1e5.csv:
	randdataset.exe -c -d 2 -n 100000 > $@

a2d1e5.csv:
	randdataset.exe -a -d 2 -n 100000 > $@

i2d1e6.csv:
	randdataset.exe -i -d 2 -n 1000000 > $@

c2d1e6.csv:
	randdataset.exe -c -d 2 -n 1000000 > $@

a2d1e6.csv:
	randdataset.exe -a -d 2 -n 1000000 > $@

clean:
	rm -rf $(datafiles)
