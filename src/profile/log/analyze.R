#
# Diagrams for Skyline Operator Evaluation
#

setwd("c:/hannes/skyline/src/profile/log/")

sky.readfile <- function(filename) {
	data = read.csv(filename);

	data$dist <- factor(substring(data$table,1,1));
	data$seed <- factor(substring(data$table,9,9));
	data$basetable <- factor(substring(data$table,1,7));
	
	return (data);
}


data = sky.readfile("master.csv");
d=aggregate(data$total, by=list(data$method, data$inrows, data$dim, data$dist), FUN = mean);
colnames(d) <- c("method", "rows", "dim", "dist", "total");

str(data)
levels(data$method)

# pdf(file = "allplots.pdf", encoding="ISOLatin1", onefile = TRUE);
# postscript(file="allplots.ps", onefile = TRUE);

skyplot.pdf <- function(filename) {
	# todo define size
	pdf(file = paste(filename, ".pdf", sep=""), encoding="ISOLatin1", onefile = FALSE)
}

skyplot.off <- function() {
	dev.off(dev.cur());
}

##
## 2 dim presort vs. sfs append, bnl append, sort
##

skyplot.presort <- function(dist) {
sel <- d$dist == dist & d$method == "sfs.append" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100 | d$dist == dist & d$method == "select" & d$rows >= 100 & d$rows <= 100000;
plot(d$rows[sel], 0.001 * d$total[sel], log="xy", type="n", xlab="# Tuples", ylab="Time (sec)");

sel <- d$dist == dist & d$method == "presort" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100;
lines(d$rows[sel], 0.001 * d$total[sel], lty="solid")
points(d$rows[sel], 0.001 * d$total[sel], pch="x")

sel <- d$dist == dist & d$method == "sfs.append" & d$dim == 2 & d$rows >= 100;
lines(d$rows[sel], 0.001 * d$total[sel], lty="dotted")
points(d$rows[sel], 0.001 * d$total[sel], pch=24);

sel <- d$dist == dist & d$method == "bnl.append" & d$dim == 2 & d$rows >= 100;
lines(d$rows[sel], 0.001 * d$total[sel], lty="solid")
points(d$rows[sel], 0.001 * d$total[sel], pch=22);

sel <- d$dist == dist & d$method == "sort" & d$dim == 2 & d$rows >= 100 & d$rows <= 100000;
lines(d$rows[sel], 0.001 * d$total[sel], lty="dashed")
points(d$rows[sel], 0.001 * d$total[sel], pch=19);

sel <- d$dist == dist & d$method == "select" & d$rows >= 100 & d$rows <= 100000;
lines(d$rows[sel], 0.001 * d$total[sel], lty="solid", lwd=2)
points(d$rows[sel], 0.001 * d$total[sel], pch=19);

sel <- d$dist == dist & d$method == "presort.idx" & d$rows >= 100 & d$rows <= 100000;
lines(d$rows[sel], 0.001 * d$total[sel], lty="solid")
points(d$rows[sel], 0.001 * d$total[sel], pch="o");

# bnl, sfs, presort, sort, select
legend("topleft", c("bnl", "sfs", "2 dim w/ sort", "2 dim w/ index", "2 dim sort only", "select only"), lty=c("solid", "dotted", "solid", "solid", "dashed", "solid"), pch=c("\x16", "\x18", "x", "o", "\x13", "\x13"), lwd=c(1,1,1,1,1,2), inset=0.05, bty="n"); 

# presort = sfs, bnl an order of magnitute faster
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("presort-", dist, sep=""));
	skyplot.presort(dist);
	skyplot.off();
}


##
## 2 dim presort vs. sfs append, bnl append, sort (scaled to sort)
##

skyplot.presortsort <- function(dist) {
sel <- d$dist == dist & d$method == "sfs.append" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100 | d$dist == dist & d$method == "select" & d$rows >= 100 & d$rows <= 100000;
ssel <- d$dist == dist & d$method == "sort" & d$dim ==2 & d$rows >= 100 & d$rows <= 100000;
plot(d$rows[sel], d$total[sel] / d$total[ssel], log="x", type="n", xlab="# Tuples", ylab="Time (relative)", ylim=c(0.1,1.5));

sel <- d$dist == dist & d$method == "presort" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100;
lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="solid")
points(d$rows[sel], d$total[sel] / d$total[ssel], pch="x")

sel <- d$dist == dist & d$method == "presort.idx" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100;
lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="solid")
points(d$rows[sel], d$total[sel] / d$total[ssel], pch="o")

sel <- d$dist == dist & d$method == "sfs.append" & d$dim == 2 & d$rows >= 100;
lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="dotted")
points(d$rows[sel], d$total[sel] / d$total[ssel], pch=24);

sel <- d$dist == dist & d$method == "bnl.append" & d$dim == 2 & d$rows >= 100;
lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="solid")
points(d$rows[sel], d$total[sel] / d$total[ssel], pch=22);

sel <- d$dist == dist & d$method == "sort" & d$dim == 2 & d$rows >= 100 & d$rows <= 100000;
lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="dashed")
points(d$rows[sel], d$total[sel] / d$total[ssel], pch=19);

sel <- d$dist == dist & d$method == "sfs.index.append" & d$dim == 2 & d$rows >= 100 & d$rows <= 100000;
lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="dotted")
points(d$rows[sel],d$total[sel] / d$total[ssel], pch="o");

#sel <- d$dist == dist & d$method == "select" & d$rows >= 100 & d$rows <= 100000;
#lines(d$rows[sel], d$total[sel] / d$total[ssel], lty="solid", lwd=2)
#points(d$rows[sel], d$total[sel] / d$total[ssel], pch=19);

# bnl, sfs, presort, sort, select
legend("bottomleft", c("bnl", "sfs", "sfs w/ index", "2 dim w/ sort", "2 dim w/ index", "2 dim sort only"), lty=c("solid", "dotted", "dotted", "solid", "solid", "dashed"), pch=c("\x16", "\x18", "o", "x", "o", "\x13"), inset=0.05, bty="n"); 
# presort = sfs, bnl an order of magnitute faster
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("presort-sort-", dist, sep=""));
	skyplot.presortsort (dist);
	skyplot.off();
}



##
## for 100.000 vs dim
##

skyplot.1e5 <- function(dist) {
rows <- 100000;
#title <- sprintf("%s%d", dist, rows);

sel <- d$rows == rows & d$dist == dist & d$dim > 1;
plot(d$dim[sel], 0.001 * d$total[sel], log="y", type="n", xlab="# Dimensions", ylab="Time (sec)");

sel <- d$rows == rows & d$dist == dist & d$method == "sql";
lines(d$dim[sel], 0.001 * d$total[sel], lty="solid")
points(d$dim[sel], 0.001 * d$total[sel], pch="x")

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.append";
lines(d$dim[sel], 0.001 * d$total[sel], lty="solid")
points(d$dim[sel], 0.001 * d$total[sel], pch=22)

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.append";
lines(d$dim[sel], 0.001 * d$total[sel], lty="dotted")
points(d$dim[sel], 0.001 * d$total[sel], pch=24)

sel <- d$rows == rows & d$dist == dist & d$method == "select";
abline(h=0.001 * d$total[sel], lwd=1, col="lightgray")

sel <- d$rows == rows & d$dist == dist & d$method == "sort" & d$dim > 1;
lines(d$dim[sel], 0.001 * d$total[sel], lty="dashed")
points(d$dim[sel], 0.001 * d$total[sel], pch=19)

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.index.append" & d$rows >= 100 & d$rows <= 100000;
lines(d$dim[sel], 0.001 * d$total[sel], lty="solid")
points(d$dim[sel], 0.001 * d$total[sel], pch="o")

# bnl, sfs, presort, sort, select
legend("topleft", c("bnl", "sfs", "sfs + index", "sql", "sort"), lty=c("solid", "dotted", "solid", "solid", "dashed"), pch=c("\x16", "\x18", "o", "x", "\x13"), inset=0.05, bty="n"); 
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("1e5-", dist, sep=""));
	skyplot.1e5 (dist);
	skyplot.off();
}


skyplot.sfs.index <- function(dist) {
rows <- 100000;
sel <- d$rows == rows & d$dist == dist & d$dim > 1;
plot(d$dim[sel], 0.001 * d$total[sel], log="y", type="n", xlab="# Dimensions", ylab="Time (sec)");

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.index.append" & d$rows >= 100 & d$rows <= 100000;
lines(d$dim[sel], 0.001 * d$total[sel], lty="dotted")
points(d$dim[sel], 0.001 * d$total[sel], pch=22)

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.index.prepend" & d$rows >= 100 & d$rows <= 100000;
lines(d$dim[sel], 0.001 * d$total[sel], lty="dotted")
points(d$dim[sel], 0.001 * d$total[sel], pch="x")

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.index.ranked" & d$rows >= 100 & d$rows <= 100000;
lines(d$dim[sel], 0.001 * d$total[sel], lty="dotted")
points(d$dim[sel], 0.001 * d$total[sel], pch=24)


# bnl, sfs, presort, sort, select
legend("topleft", c("sfs + index append", "sfs + index prepend", "sfs + index entropy"), lty=c("dotted", "dotted", "dotted"), pch=c("\x16", "x", "\x18"), inset=0.05, bty="n"); 
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("sfs-index-", dist, sep=""));
	skyplot.sfs.index(dist);
	skyplot.off();
}


skyplot.bnlwp <- function(dist, rows) {
par(col="black", lty="solid");
ssel <- d$rows == rows & d$dist == dist & d$method == "bnl.append" & d$dim > 1;
plot(d$dim[ssel], d$total[ssel] / d$total[ssel], log="x", type="n", xlab="# Dimensions", ylab="Time (relative)", ylim=c(0.5,2));

abline(h=1, lwd=1, col="lightgray")

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.prepend";
par(lty="solid", pch="x");
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.ranked";
par(lty="solid", pch=24);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.ef.append";
par(lty="solid", pch=19);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.append";
par(lty="dotted", pch=22);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

# bnl, sfs, presort, sort, select
legend("topright", c("bnl append", "bnl prepend", "bnl entropy", "bnl+ef append", "sfs"), 
			lty=c("solid", "solid", "solid", "solid", "dotted"), pch=c("", "x", "\x18", "\x13", "\x16"), col=c("lightgray","black","black", "black", "black"), inset=0.05, bty="n"); 
box()
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("bnl-wp-1e5-", dist, sep=""));
	skyplot.bnlwp(dist, 100000);
	skyplot.off();
}


##
##
##

skyplot.sfswp <- function(dist, rows) {
par(col="black", lty="solid");
ssel <- d$rows == rows & d$dist == dist & d$method == "sfs.append" & d$dim > 1;
plot(d$dim[ssel], d$total[ssel] / d$total[ssel], log="x", type="n", xlab="# Dimensions", ylab="Time (relative)", ylim=c(0.1,2));

abline(h=1, lwd=1, col="lightgray", lty="dotted")

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.prepend";
par(lty="dotted", pch="x");
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.ranked";
par(lty="dotted", pch=24);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.ef.append";
par(lty="dotted", pch=19);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.append";
par(lty="solid", pch=22);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.index.append"
ssel <- d$rows == rows & d$dist == dist & d$method == "sfs.append" & d$dim > 1 & d$dim <= 6;
par(lty="dotted", pch="o");
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);

# bnl, sfs, presort, sort, select
legend("topleft", c("sfs append", "sfs prepend", "sfs entropy", "sfs+ef append", "sfs + index append", "bnl append"),
	lty=c("dotted", "dotted", "dotted", "dotted", "dotted", "solid"),
	pch=c("", "x", "\x18", "\x13", "o", "\x16"), col=c("lightgray","black","black", "black", "black", "black"), inset=0.05, bty="n"); 
box()
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("sfs-wp-1e5-", dist, sep=""));
	skyplot.sfswp (dist, 100000);
	skyplot.off();
}

dev.off(dev.cur());





##
## tuple cmps
##

d=aggregate(data$skyline.cmps.tuples, by=list(data$method, data$inrows, data$dim, data$dist), FUN = mean);
colnames(d) <- c("method", "rows", "dim", "dist", "cmps");

rows <- 100000;

ssel <- d$rows == rows & d$dist == dist & d$method == "bnl.append" & d$dim > 1;
sel <- ssel;
plot(d$dim[sel], d$cmps[sel] / d$cmps[ssel], type="n", xlab = "# Dimensions", ylab = "# Tuple Comparisons", ylim=c(0.2,2))

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.append" & d$dim > 1;
par(lty="solid", pch=22);
lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.prepend" & d$dim > 1;
par(lty="solid", pch="x");
lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "bnl.ranked" & d$dim > 1;
par(lty="solid", pch=24);
lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.append" & d$dim > 1;
par(lty="dotted", pch=22);
lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.prepend" & d$dim > 1;
par(lty="dotted", pch="x");
lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);

sel <- d$rows == rows & d$dist == dist & d$method == "sfs.ranked" & d$dim > 1;
par(lty="dotted", pch=24);
lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);




##
## field cmps vs. tuple cmps
##

d=aggregate(data$skyline.cmps.fields / data$skyline.cmps.tuples, by=list(data$method, data$inrows, data$dim, data$dist), FUN = mean);
colnames(d) <- c("method", "rows", "dim", "dist", "cmps");

skyplot.cmppertuple <- function(method, ...) {
rows <- 100000;
dist <- "c";
sel <- d$rows == rows & d$method == method;
plot(d$dim[sel], d$cmps[sel] , type="n", xlab = "# Dimensions", ylab = "# Tuple Comparisons", ...);

sel <- d$rows == rows & d$dist == dist & d$method == method;
par(lty="solid", pch=24);
lines(d$dim[sel], d$cmps[sel] ); points(d$dim[sel], d$cmps[sel]);

dist="a";
sel <- d$rows == rows & d$dist == dist & d$method == method;
par(lty="solid", pch="x");
lines(d$dim[sel], d$cmps[sel] ); points(d$dim[sel], d$cmps[sel]);

dist="i";
sel <- d$rows == rows & d$dist == dist & d$method == method;
par(lty="solid", pch=22);
lines(d$dim[sel], d$cmps[sel] ); points(d$dim[sel], d$cmps[sel]);

legend("bottomright", c("corr", "indep", "anti"), 
			lty=c("solid", "solid", "solid"), pch=c("\x18", "\x16", "x"), inset=0.05, bty="n"); 
}


skyplot.cmppertuple("bnl.append", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.prepend", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.ranked", ylim=c(2,3.25));

skyplot.cmppertuple("bnl.ef.append", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.ef.prepend", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.ef.ranked", ylim=c(2,3.25));

skyplot.cmppertuple("sfs.append", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.prepend", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.ranked", ylim=c(2,3.5));

skyplot.cmppertuple("sfs.ef.append", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.ef.prepend", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.ef.ranked", ylim=c(2,3.5));

skyplot.cmppertuple("sfs.index.append")
skyplot.cmppertuple("sfs.index.prepend")
skyplot.cmppertuple("sfs.index.ranked")



###
###
### EFSLOTS
###
###

data = sky.readfile("ef.csv")
d=aggregate(data$total, by=list(data$method, data$inrows, data$dim, data$dist, data$elimfilter.windowslots), FUN = mean);
colnames(d) <- c("method", "rows", "dim", "dist", "efslots", "total");


str(data)
str(d)
levels(d$method)


rows <- 10000;
dim <- 4;
method <- "bnl.ef.append.ranked";
sel <- d$dim == dim & d$rows == rows & d$method == method;
plot(d$efslots[sel], d$total[sel], type="n", xlab = "# EF Slots", ylab = "Time (sec)");

dist <- "c";
sel <- d$dim == dim & d$dist == dist & d$rows == rows & d$method == method;
par(lty="solid", pch=24);
lines(d$efslots[sel], d$total[sel]); points(d$efslots[sel], d$total[sel]);

dist="a";
sel <- d$dim == dim & d$dist == dist & d$rows == rows & d$method == method;
par(lty="solid", pch="x");
lines(d$efslots[sel], d$total[sel]); points(d$efslots[sel], d$total[sel]);

dist="i";
sel <- d$dim == dim & d$dist == dist & d$rows == rows & d$method == method;
par(lty="solid", pch=22);
lines(d$efslots[sel], d$total[sel]); points(d$efslots[sel], d$total[sel]);

legend("bottomright", c("corr", "indep", "anti"), 
			lty=c("solid", "solid", "solid"), pch=c("\x18", "\x16", "x"), inset=0.05, bty="n"); 



for (method in c("bnl.ef.append.ranked", "sfs.ef.append.ranked")) {
for (dist in c("i", "c", "a")) {


skyplot.pdf(paste(method, "-", dist, "-vs-dim", sep=""));

#method <- "sfs.ef.append.ranked";
#dist <- "a";

rows <- 10000;
sel <- d$rows == rows & d$method == method & d$dist == dist;
ssel <- d$rows == rows & d$method == method & d$dist == dist & d$efslots == 100;
palette("default")
plot(d$dim[ssel], d$total[ssel] / d$total[ssel], type="n", xlab = "# Dimensons", ylab = "Time (relative)");

palette(rainbow(length(levels(factor(d$efslots)))))

colidx=0;
for (efslots in levels(factor(d$efslots))) {
sel <- d$efslots == efslots & d$dist == dist & d$rows == rows & d$method == method;
colidx=colidx+1;
par(lty="solid", pch=24);
par(col=colidx);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);
}

legend("topright", 
	paste("ef slots", levels(factor(d$efslots))),
	lty=rep(c("solid"), length(levels(factor(d$efslots)))),
	col=1:length(levels(factor(d$efslots))),
	pch=rep(c("\x18"), length(levels(factor(d$efslots)))),
	inset=0.05, bty="n")

palette("default")
par(col="black")
box()

skyplot.off();

}
}



##
##
## WINDOW SLOTS
##
##

data = sky.readfile("wnd.csv")
d=aggregate(data$total, by=list(data$method, data$inrows, data$dim, data$dist, data$skyline.windowslots), FUN = mean);
colnames(d) <- c("method", "rows", "dim", "dist", "efslots", "total");


str(data)
str(d)
levels(d$method)



for (method in c("bnl.append", "sfs.append")) {
for (dist in c("a")) {


skyplot.pdf(paste(method, "-", dist, "-vs-dim", sep=""));

method <- "sfs.append";
dist <- "a";

rows <- 10000;
sel <- d$rows == rows & d$method == method & d$dist == dist;
ssel <- d$rows == rows & d$method == method & d$dist == dist & d$efslots == 100;
palette("default")
plot(d$dim[ssel ], d$total[ssel] / d$total[ssel], type="n", xlab = "# Dimensions", ylab = "Time (relative)");

palette(rainbow(length(levels(factor(d$efslots)))))

colidx=0;
for (efslots in levels(factor(d$efslots))) {
sel <- d$efslots == efslots & d$dist == dist & d$rows == rows & d$method == method;
colidx=colidx+1;
par(lty="solid", pch=24);
par(col=colidx);
lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);
}

legend("bottomright", 
	paste("wnd slots", levels(factor(d$efslots))),
	lty=rep(c("solid"), length(levels(factor(d$efslots)))),
	col=1:length(levels(factor(d$efslots))),
	pch=rep(c("\x18"), length(levels(factor(d$efslots)))),
	inset=0.05, bty="n")

palette("default")
par(col="black")
box()

skyplot.off();

}
}
