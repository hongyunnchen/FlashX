% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/SVD.R
\docType{methods}
\name{svd}
\alias{fm.svd}
\alias{svd}
\alias{svd,fm-method}
\title{Compute the singular-value decomposition on a large matrix.}
\usage{
fm.svd(x, nu = min(n, p), nv = min(n, p), tol = 1e-08)

\S4method{svd}{fm}(x, nu = min(n, p), nv = min(n, p), LINPACK = FALSE)
}
\arguments{
\item{x}{a FlashR matrix}

\item{nu}{the number of left singluar vectors to be computed.}

\item{nv}{the number of right singluar vectors to be computed.}

\item{tol}{Stopping criterion: the relative accuracy of the Ritz value
is considered acceptable if its error is less than 'tol' times its estimated
value. If this is set to zero then machine precision is used.}
}
\value{
Returns a list with three entries
  \item{d}{ max(nu, nv) approximate singular values}
  \item{u}{ nu approximate left singular vectors (only when right_only=FALSE)}
  \item{v}{ nv approximate right singular vectors}
}
\description{
The difference between \code{svd} and \code{fm.svd} is that \code{fm.svd}
allows a user-specified tol. \code{svd} computes eigenvalues in machine
precision.
}
\examples{
mat <- fm.runif.matrix(1000, 100)
res <- fm.svd(mat, nu=10, nv=0)
res <- svd(mat, nu=10, nv=0)
}
\author{
Da Zheng <dzheng5@jhu.edu>
}

