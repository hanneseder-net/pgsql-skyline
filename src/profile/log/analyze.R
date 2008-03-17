#
# Diagrams for Skyline Operator Evaluation
#

setwd("c:/hannes/skyline/src/profile/log-skyXX/")

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

##
## 2 dim presort vs. sfs append, bnl append, sort
##

#pdf(encoding="ISOLatin1", file="test.pdf")
dist = "a";

sel <- d$dist == dist & d$method == "sfs.append" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100 | d$dist == dist & d$method == "select" & d$rows >= 100 & d$rows <= 100000;
plot(d$rows[sel], 0.001 * d$total[sel], log="xy", type="n", xlab="# Tuples", ylab="Time (sec)");

sel <- d$dist == dist & d$method == "presort" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100;
lines(d$rows[sel], 0.001 * d$total[sel], lty="solid")
points(d$rows[sel], 0.001 * d$total[sel], pch="x")

#TODO: check this again
#sel <- d$dist == dist & d$method == "presort.idx" & d$dim == 2 & d$rows <= 100000 & d$rows >= 100;
#lines(d$rows[sel], 0.001 * d$total[sel], lty="solid")
#points(d$rows[sel], 0.001 * d$total[sel], pch="x")

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

# bnl, sfs, presort, sort, select
legend("topleft", c("bnl", "sfs", "2 dim w/ sort", "2 dim sort only", "select only"), lty=c("solid", "dotted", "solid", "dashed", "solid"), pch=c("\x16", "\x18", "x", "\x13", "\x13"), lwd=c(1,1,1,1,2), inset=0.05, bty="n"); 

# presort = sfs, bnl an order of magnitute faster
#dev.off()

##
## for 100.000 vs dim
##

dist <- "a";
rows <- 100000;

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


