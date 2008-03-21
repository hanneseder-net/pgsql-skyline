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

#str(data)
#levels(data$method)

# pdf(file = "allplots.pdf", encoding="ISOLatin1", onefile = TRUE);
# postscript(file="allplots.ps", onefile = TRUE);

skyplot.pdf <- function(filename) {
	# todo define size
	pdf(file = paste(filename, ".pdf", sep=""), encoding="ISOLatin1", onefile = FALSE)
	assign("skyplot.title", filename, envir = .GlobalEnv);
}

skyplot.off <- function() {
	title(main=get("skyplot.title", envir = .GlobalEnv), col="black");
	dev.off();
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
	skyplot.pdf(paste("presort-rel-", dist, sep=""));
	skyplot.presortsort (dist);
	skyplot.off();
}

skyplot.pch.wp <- function(wp) {
	pch <- switch(wp,
		append = "\x16",
		prepend = "x",
		entropy = "\x18",
		random = "o",
		"?"
		);
	return (pch);
}

skyplot.pch <- function(method) {
	parts <- strsplit(method, ".", fixed=TRUE)[[1]];

	if (length(parts) == 2 || length(parts) == 3) {
		return (skyplot.pch.wp(parts[length(parts)]));
	}
	else if (length(parts) == 4) {
		if (parts[1] == "sfs" && parts[2] == "index") {
			return (skyplot.pch.wp(parts[length(parts)]));
		}
		else if (parts[2] == "ef") {
			if (parts[3] == parts[4]) {
				return (skyplot.pch.wp(parts[length(parts)]));
			}
			else {
				warning("mixed wp not yet supported");
				return (skyplot.pch.wp(parts[3]));
			}	
		}
		else
			warning("unknown method");
	}
	else if (length(parts) == 5) {
		if (parts[1] == "sfs" && parts[2] == "index" && parts[3] == "ef") {
			return (skyplot.pch.wp(parts[length(parts)]));
		}
	}
	else
		warning("unknown method");
}

# skyplot.pch("sfs.index.ef.append.append");

skyplot.col <- function(method) {
	parts <- strsplit(method, ".", fixed=TRUE)[[1]];
	method <- parts[1];

	if (length(parts) > 2) {
		ef <- parts[2] == "ef" || parts[3] == "ef";
	}
	else {
		ef <- parts[2] == "ef";
	}

	index <- parts[2] == "index";

	if (ef == TRUE) method <- paste(method, ".ef", sep="");
	if (index == TRUE) method <- paste(method, ".index", sep="");

	col <- switch(method,
		bnl = "black",
		sfs = "green",
		bnl.ef = "red",
		sfs.ef = "blue",
		sfs.index = "orange",
		sfs.ef.index = "blue",
		"pink"
		);

	return (col);
}

##
## all (sql, sort, bnl, sfs ...)
##

skyplot.alltimeabs <- function(dist, rows) {
#title <- sprintf("%s%d", dist, rows);

sel <- d$rows == rows & d$dist == dist & d$dim > 1;
plot(d$dim[sel], 0.001 * d$total[sel], log="y", type="n", xlab="# Dimensions", ylab="Time (sec)");

sel <- d$rows == rows & d$dist == dist & d$method == "sql";
lines(d$dim[sel], 0.001 * d$total[sel], lty="solid")
points(d$dim[sel], 0.001 * d$total[sel], pch="x")

sel <- d$rows == rows & d$dist == dist & d$method == "select";
abline(h=0.001 * d$total[sel], lwd=1, col="lightgray")

sel <- d$rows == rows & d$dist == dist & d$method == "sort" & d$dim > 1;
lines(d$dim[sel], 0.001 * d$total[sel], lty="solid")
points(d$dim[sel], 0.001 * d$total[sel], pch=19)

#sel <- d$rows == rows & d$dist == dist & d$method == "sfs.index.append" & d$rows >= 100 & d$rows <= 100000;
#lines(d$dim[sel], 0.001 * d$total[sel], lty="solid")
#points(d$dim[sel], 0.001 * d$total[sel], pch="o")

for (method in c(
			#"sfs.append", 
			#"sfs.prepend", #"sfs.entropy", "sfs.random",
			"bnl.append"#, "bnl.prepend", "bnl.entropy", "bnl.random",
			#"bnl.ef.append.append", "bnl.ef.prepend.prepend", "bnl.ef.entropy.entropy", "bnl.ef.random.random",
			#"sfs.ef.append.append", "sfs.ef.prepend.prepend", "sfs.ef.entropy.entropy", "sfs.ef.random.random"
	)) {
	sel <- d$rows == rows & d$dist == dist & d$method == method;
	par(lty="solid", pch=skyplot.pch(method), col=skyplot.col(method));
	lines(d$dim[sel], 0.001 * d$total[sel]); points(d$dim[sel], 0.001 * d$total[sel]);
}

par(col="black");

# bnl, sfs, presort, sort, select
#legend("topleft", c("bnl append", "sfs", "sfs + index", "sql", "sort"), lty=c("solid", "dotted", "solid", "solid", "dashed"), pch=c("\x16", "\x18", "o", "x", "\x13"), inset=0.05, bty="n"); 
legend("topleft", c("bnl append", "sql", "sort"), lty=c("solid", "solid", "solid"), pch=c("\x16", "x", "\x13"), inset=0.05, bty="n"); 

box();
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("all-dim-", dist, "-", sprintf("%d", rows), sep=""));
	rows <- 100000;
	dist <- "i";
	skyplot.alltimeabs(dist, rows);
	skyplot.off();
}


##
## sfs.index
##

skyplot.sfs.index <- function(dist) {
par(col="black");
rows <- 100000;
sel <- d$rows == rows & d$dist == dist & d$dim > 1 & d$dim <= 6 & 
	(d$method == "sfs.index.ef.append.append" | d$method == "sfs.index.prepend");
plot(d$dim[sel], 0.001 * d$total[sel], log="y", type="n", xlab="# Dimensions", ylab="Time (sec)");

sel <- d$rows == rows & d$dist == dist & d$dim > 1 & d$method == "sfs.index.append";

for (method in c(
		"sfs.index.append", "sfs.index.prepend", "sfs.index.entropy", "sfs.index.random",
		"sfs.index.ef.append.append", "sfs.index.ef.prepend.prepend", "sfs.index.ef.entropy.entropy", "sfs.index.ef.random.random")) {
	sel <- d$rows == rows & d$dist == dist & d$dim > 1 & d$dim <= 6 & d$method == method  ;
	par(lty="solid", pch=skyplot.pch(method), col=skyplot.col(method));
	lines(d$dim[sel], 0.001 * d$total[sel]); points(d$dim[sel], 0.001 * d$total[sel]);
}
par(col="black")
# bnl, sfs, presort, sort, select
legend("topleft", 
	paste(rep(c("sf+index", "sfs+index+ef"),each=4), c("append", "prepend", "entropy", "random")),
	lty=rep(c("solid"),8), pch=rep(c("\x16", "x", "\x18","o"),2), col=rep(c("orange","blue"),each=4), inset=0.05, bty="n"); 
box();
}

for (dist in c("i", "c", "a")) {
	skyplot.pdf(paste("sfs-index-", dist, sep=""));
	skyplot.sfs.index(dist);
	skyplot.off();
}

##
## 
##

skyplot.wp <- function(d, ssel, dist, rows,legendpos) {

for (method in c(
			"sfs.append", "sfs.prepend", "sfs.entropy", "sfs.random",
			"bnl.append", "bnl.prepend", "bnl.entropy", "bnl.random",
			"bnl.ef.append.append", "bnl.ef.prepend.prepend", "bnl.ef.entropy.entropy", "bnl.ef.random.random",
			"sfs.ef.append.append", "sfs.ef.prepend.prepend", "sfs.ef.entropy.entropy", "sfs.ef.random.random")) {
	sel <- d$rows == rows & d$dist == dist & d$method == method;
	par(lty="solid", pch=skyplot.pch(method), col=skyplot.col(method));
	lines(d$dim[sel], d$total[sel] / d$total[ssel]); points(d$dim[sel], d$total[sel] / d$total[ssel]);
}

par(col="black");
legend(legendpos,
	paste(rep(c("bnl","bnl+ef", "sfs", "sfs+ef"),each=4), c("append", "prepend", "entropy", "random")),
	lty="solid",
	pch=rep(c("\x16", "x", "\x18", "o"), 4),
	col=rep(c("black", "red", "green", "blue"), each=4), inset=0.05, bty="n", ncol=2)
box()
}

skyplot.bnlwp <- function(dist, rows) {

par(col="black", lty="solid");
ssel <- d$rows == rows & d$dist == dist & d$method == "bnl.append" & d$dim > 1;
plot(d$dim[ssel], d$total[ssel] / d$total[ssel], type="n", xlab="# Dimensions", ylab="Time (relative)", ylim=c(0.25,2));

skyplot.wp(d, ssel, dist,rows, "topright");
}

for (rows in c(100, 1000, 10000, 100000)) {
	for (dist in c("i", "c", "a")) {
		skyplot.pdf(paste("bnl-wp-dim", "-", dist, "-", sprintf("%d", rows), sep=""));
		skyplot.bnlwp(dist, rows);
		skyplot.off();
	}
}


##
## relative runtime vs. rows (comparing bnl, sfs, bnl+ef, bnl+ef and windowpolicies)
##

skyplot.wptups <- function(d, ssel, dist, dim, legendpos) {

for (method in c(
			"sfs.append", "sfs.prepend", "sfs.entropy", "sfs.random",
			"bnl.append", "bnl.prepend", "bnl.entropy", "bnl.random",
			"bnl.ef.append.append", "bnl.ef.prepend.prepend", "bnl.ef.entropy.entropy", "bnl.ef.random.random",
			"sfs.ef.append.append", "sfs.ef.prepend.prepend", "sfs.ef.entropy.entropy", "sfs.ef.random.random")) {
	sel <- d$dim == dim & d$dist == dist & d$method == method & d$rows >= 100;
	par(lty="solid", pch=skyplot.pch(method), col=skyplot.col(method));
	lines(d$rows[sel], d$total[sel] / d$total[ssel]); points(d$rows[sel], d$total[sel] / d$total[ssel]);
}

par(col="black");
legend(legendpos,
	paste(rep(c("bnl","bnl+ef", "sfs", "sfs+ef"),each=4), c("append", "prepend", "entropy", "random")),
	lty="solid",
	pch=rep(c("\x16", "x", "\x18", "o"), 4),
	col=rep(c("black", "red", "green", "blue"), each=4), inset=0.05, bty="n", ncol=2)
box()
}

skyplot.bnlwptups <- function(dist, dim) {

par(col="black", lty="solid");
ssel <- d$dim == dim & d$dist == dist & d$method == "bnl.append" & d$rows >= 100;
plot(d$rows[ssel], d$total[ssel] / d$total[ssel], log="x", type="n", xlab="# Tuples", ylab="Time (relative)", ylim=c(0.25,2));

skyplot.wptups(d, ssel, dist, dim, "topright");
}

for (dist in c("i", "c", "a")) {
	for (dim in 2:15) {
	skyplot.pdf(paste("bnl-wp-rows", "-", dist, "-", sprintf("%.2d", dim), sep=""));
	skyplot.bnlwptups(dist, dim);
	skyplot.off();
	}
}





##
##
##

#skyplot.sfswp <- function(dist, rows) {
#par(col="black", lty="solid");
#ssel <- d$rows == rows & d$dist == dist & d$method == "sfs.append" & d$dim > 1;
#plot(d$dim[ssel], d$total[ssel] / d$total[ssel], type="n", xlab="# Dimensions", ylab="Time (relative)", ylim=c(0.1,2));
#
#skyplot.wp(d, ssel, dist,rows,"topleft");
#}
#
#for (dist in c("i", "c", "a")) {
#	skyplot.pdf(paste("sfs-wp-1e5-", dist, sep=""));
#	skyplot.sfswp (dist, 100000);
#	skyplot.off();
#}




##
## tuple cmps
##

#d=aggregate(data$skyline.cmps.tuples, by=list(data$method, data$inrows, data$dim, data$dist), FUN = mean);
#colnames(d) <- c("method", "rows", "dim", "dist", "cmps");
#
#rows <- 100000;
#
#ssel <- d$rows == rows & d$dist == dist & d$method == "bnl.append" & d$dim > 1;
#sel <- ssel;
#plot(d$dim[sel], d$cmps[sel] / d$cmps[ssel], type="n", xlab = "# Dimensions", ylab = "# Tuple Comparisons", ylim=c(0.2,2))
#
#sel <- d$rows == rows & d$dist == dist & d$method == "bnl.append" & d$dim > 1;
#par(lty="solid", pch=22);
#lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);
#
#sel <- d$rows == rows & d$dist == dist & d$method == "bnl.prepend" & d$dim > 1;
#par(lty="solid", pch="x");
#lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);
#
#sel <- d$rows == rows & d$dist == dist & d$method == "bnl.ranked" & d$dim > 1;
#par(lty="solid", pch=24);
#lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);
#
#sel <- d$rows == rows & d$dist == dist & d$method == "sfs.append" & d$dim > 1;
#par(lty="dotted", pch=22);
#lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);
#
#sel <- d$rows == rows & d$dist == dist & d$method == "sfs.prepend" & d$dim > 1;
#par(lty="dotted", pch="x");
#lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);
#
#sel <- d$rows == rows & d$dist == dist & d$method == "sfs.ranked" & d$dim > 1;
#par(lty="dotted", pch=24);
#lines(d$dim[sel], d$cmps[sel] / d$cmps[ssel]); points(d$dim[sel], d$cmps[sel] / d$cmps[ssel]);


dtc=(data$skyline.cmps.tuples, by=list(data$method, data$inrows, data$dim, data$dist), FUN = mean);
colnames(dtc) <- c("method", "rows", "dim", "dist", "cmps");

dim <- 4;
dist <- "a";
sel <- !is.na(dtc$cmps) & dtc$dim == dim & dtc$method != "" & dtc$rows == 10000
	dtc$method %in%
		c(
			"sfs.append", "sfs.prepend", "sfs.entropy", "sfs.random",
			"bnl.append", "bnl.prepend", "bnl.entropy", "bnl.random",
			"bnl.ef.append.append", "bnl.ef.prepend.prepend", "bnl.ef.entropy.entropy", "bnl.ef.random.random",
			"sfs.ef.append.append", "sfs.ef.prepend.prepend", "sfs.ef.entropy.entropy", "sfs.ef.random.random");
boxplot((dtc$cmps[sel] / dtc$rows[sel]) ~ unclass(dtc$method[sel]), ylab="# Tuple Comps")

##
## field cmps vs. tuple cmps
##

d=aggregate(data$skyline.cmps.fields / data$skyline.cmps.tuples, by=list(data$method, data$inrows, data$dim, data$dist), FUN = mean);
colnames(d) <- c("method", "rows", "dim", "dist", "cmps");

skyplot.cmppertuple <- function(method, ...) {
skyplot.pdf(paste("fld-tup-cmps-", method, sep=""));

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
skyplot.off();
}


skyplot.cmppertuple("bnl.append", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.prepend", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.ranked", ylim=c(2,3.25));

skyplot.cmppertuple("bnl.ef.append.append", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.ef.prepend.prepend", ylim=c(2,3.25));
skyplot.cmppertuple("bnl.ef.ranked", ylim=c(2,3.25));

skyplot.cmppertuple("sfs.append", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.prepend", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.ranked", ylim=c(2,3.5));

skyplot.cmppertuple("sfs.ef.append.append", ylim=c(2,3.5));
skyplot.cmppertuple("sfs.ef.prepend.prepend", ylim=c(2,3.5));
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


#str(data)
#str(d)
#levels(d$method)


#rows <- 10000;
#dim <- 4;
#method <- "bnl.ef.append.ranked";
#sel <- d$dim == dim & d$rows == rows & d$method == method;
#plot(d$efslots[sel], d$total[sel], type="n", xlab = "# EF Slots", ylab = "Time (sec)");
#
#dist <- "c";
#sel <- d$dim == dim & d$dist == dist & d$rows == rows & d$method == method;
#par(lty="solid", pch=24);
#lines(d$efslots[sel], d$total[sel]); points(d$efslots[sel], d$total[sel]);
#
#dist="a";
#sel <- d$dim == dim & d$dist == dist & d$rows == rows & d$method == method;
#par(lty="solid", pch="x");
#lines(d$efslots[sel], d$total[sel]); points(d$efslots[sel], d$total[sel]);
#
#dist="i";
#sel <- d$dim == dim & d$dist == dist & d$rows == rows & d$method == method;
#par(lty="solid", pch=22);
#lines(d$efslots[sel], d$total[sel]); points(d$efslots[sel], d$total[sel]);
#
#legend("bottomright", c("corr", "indep", "anti"), 
#			lty=c("solid", "solid", "solid"), pch=c("\x18", "\x16", "x"), inset=0.05, bty="n"); 



for (method in c("bnl.ef.append.ranked", "sfs.ef.append.ranked")) {
for (dist in c("i", "c", "a")) {

rows <- 100000;

skyplot.pdf(paste(method, "-", rows, "-", dist, "-vs-dim", sep=""));

#method <- "sfs.ef.append.ranked";
#dist <- "a";

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
	inset=0.05, bty="n", ncol=2)

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


#str(data)
#str(d)
#levels(d$method)



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
