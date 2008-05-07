def.par <- par(no.readonly = TRUE)
data = read.csv("c:\\hannes\\skyline\\mathematica\\histo\\a2d1e5.csv", header=FALSE, col.names=c("d1", "d2"))
par(def.par);
par.mar <- par(c("mar"));

nf <- layout(matrix(c(1,0,2,3), 2, 2, byrow = TRUE), widths=c(5,2), height=c(2,5));

bins = 50
par(mar=c(0.5,par.mar[2],2,0.5))
hist(data$d1,breaks=bins-1, main="", xlab="", ylab="Frequency", axes=FALSE, horizontal=TRUE)
#axis(1, lables=TRUE)
axis(2, labels=FALSE)


data.bins = floor(data * bins)/bins
data.bc = aggregate(data.bins$d1, list(data.bins$d1, data.bins$d2), FUN=length)
colnames(data.bc) <- c("d1", "d2", "count")
maxcount = max(data.bc$count)


par(mar=c(5,par.mar[2],0.5,0.5));
plot(data, type="n", xlab="d1")
rect(data.bc$d1, data.bc$d2, data.bc$d1+(1/bins), data.bc$d2+(1/bins), col=gray(0.9-0.9*(data.bc$count/maxcount)), lty="blank")

#data.skyon = data.bc[,1:2]
data.skyon = data

data.idx = order(data.skyon$d1, data.skyon$d2);
data.ord = data.skyon[data.idx, ]


data.sky = data.ord[1,];
data.skyl = data.sky[0,];
t <- cbind(data.sky$d1,1)
colnames(t) <- colnames(data.sky)
data.skyl = rbind(data.skyl, t, data.sky)

sky = data.ord[1,];

for (i in 2:length(data.ord$d1)) {
	if (sky$d1 <= data.ord$d1[i] && sky$d2 <= data.ord$d2[i] && (sky$d1 < data.ord$d1[i] || sky$d2 < data.ord$d2[i])) {
		# sky dominates data
	}
	else if (data.ord$d1[i] <= sky$d1 && data.ord$d2[i] <= sky$d2 && (data.ord$d1[i] < sky$d1 || data.ord$d2[i] < sky$d2)) {
		# will not happen here
		# sky = data.ord[i,];
		# str(sky)
	}
	else {
		d2 <- sky$d2;
		sky = data.ord[i,];
		d1 <- sky$d1;
		t <- cbind(d1, d2);
		colnames(t) <- colnames(sky);
		data.skyl <- rbind(data.skyl, t, sky);
		data.sky = rbind(data.sky, sky)
	}
}

t <- cbind(1, sky$d2);
colnames(t) <- colnames(sky);
data.skyl <- rbind(data.skyl, t, sky);

lines(data.skyl$d1, data.skyl$d2, col="red")
#points(data.sky$d1, data.sky$d2, col="red")


par(mar=c(5,0.5,0.5,par.mar[4]));
h <- hist(data$d2, breaks=bins-1, plot=FALSE);
plot(h$counts, h$breaks[1:length(h$breaks)-1], type="n", ylab="", xlab="Frequency", axes=FALSE, xlim=c(0,max(h$counts)))
rect(rep(0, length(h$counts)), h$breaks[1:length(h$breaks)-1], h$counts,  h$breaks[2:length(h$breaks)]);
axis(1, labels=FALSE)

par(def.par)

