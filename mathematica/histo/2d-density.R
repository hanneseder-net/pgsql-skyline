skyline2d <- function(d) {
	idx = order(d$d1, d$d2);
	ord = d[idx, ]

	skyl = ord[1,];
	sky = ord[1,];

	for (i in 2:length(ord$d1)) {
		if (sky$d1 <= ord$d1[i] && sky$d2 <= ord$d2[i] && (sky$d1 < ord$d1[i] || sky$d2 < ord$d2[i])) {
			# sky dominates ord[i]
		}
		else if (ord$d1[i] <= sky$d1 && ord$d2[i] <= sky$d2 && (ord$d1[i] < sky$d1 || ord$d2[i] < sky$d2)) {
			# will not happen here
			# sky = data.ord[i,];
			# str(sky)
		}
		else {
			sky = ord[i,];
			skyl = rbind(skyl, sky);
		}
	}
	
	return (skyl);
}

skyline2dline <- function(sp) {
	sl = data.frame(d1 = c(rep(sp$d1, each=2), 1), d2 = c(1, rep(sp$d2, each=2)));

	return (sl);
}




distplot <- function(data) {

def.par <- par(no.readonly = TRUE)

par(def.par);
par.mar <- par(c("mar"));

#nf <- layout(matrix(c(1,4,2,3,0,5), 3, 2, byrow = TRUE), widths=c(5,2), height=c(2,5,2));
nf <- layout(matrix(c(1,0,2,3), 2, 2, byrow = TRUE), widths=c(5,2), height=c(2,5,2));


bins = 40

# d1 histo
par(mar=c(0.5,par.mar[2],2,0.5))
h <- hist(data$d1, breaks=bins-1, plot=FALSE);
plot(h$breaks[1:length(h$breaks)-1], h$density, type="n", ylab="Density", xlab="", axes=FALSE, xlim=c(0,1), ylim=c(0,1.75))
rect(h$breaks[1:length(h$breaks)-1], rep(0, length(h$density)), h$breaks[2:length(h$breaks)], h$density);
axis(2, labels=TRUE)

# normal dist with same mean/sd as d1
dat = min(data$d1) + (0:100 * (max(data$d1)-min(data$d1))/100);
lines(dat, dnorm(dat, mean=mean(data$d1), sd=sd(data$d1)), col="blue")

# 2d density plot
data.bins = floor(data * bins)/bins
sel = data.bins$d1 < 1 & data.bins$d2 < 1;
data.bc = aggregate(data.bins$d1[sel], list(data.bins$d1[sel], data.bins$d2[sel]), FUN=length)
colnames(data.bc) <- c("d1", "d2", "count")
maxcount = max(data.bc$count)

par(mar=c(5,par.mar[2],0.5,0.5));
plot(data, type="n", xlab="d1")
rect(data.bc$d1, data.bc$d2, data.bc$d1+(1/bins), data.bc$d2+(1/bins), col=gray(0.9-0.9*(data.bc$count/maxcount)), lty="blank")

# plot skyline
sp = skyline2d(data)
sl = skyline2dline(sp)
lines(sl , col="red")

# d2 histo
par(mar=c(5,0.5,0.5,par.mar[4]));
h <- hist(data$d2, breaks=bins-1, plot=FALSE);
plot(h$density, h$breaks[1:length(h$breaks)-1], type="n", ylab="", xlab="Density", axes=FALSE, xlim=c(0,1.75))
rect(rep(0, length(h$density)), h$breaks[1:length(h$breaks)-1], h$density,  h$breaks[2:length(h$breaks)]);
axis(1, labels=TRUE)

# normal dist with same mean/sd as d1
dat = min(data$d2) + (0:100 * (max(data$d2)-min(data$d2))/100);
lines(dnorm(dat, mean=mean(data$d2), sd=sd(data$d2)), dat, col="blue")


par(def.par)

}



setwd("c:\\hannes\\skyline\\mathematica\\histo\\");

for (dist in c("i", "c", "a")) {
	data = read.csv(paste(dist, "2d1e5.csv", sep=""), header=FALSE, col.names=c("d1", "d2"));
	pdf(file = paste("density-2d-", dist, "2d1e5.pdf", sep=""), encoding="ISOLatin1", onefile = FALSE, width=6, height=6)
	distplot(data);
	dev.off();
}

