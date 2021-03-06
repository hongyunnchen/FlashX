% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/FlashR_base.R
\name{fm.eigen}
\alias{fm.cal.residul}
\alias{fm.eigen}
\title{Eigensolver}
\usage{
fm.eigen(mul, k, n, which = "LM", sym = TRUE, options = NULL,
  env = parent.frame())

fm.cal.residul(mul, values, vectors)
}
\arguments{
\item{mul}{The function to perform the matrix-vector multiplication.}

\item{k}{Integer. The number of eigenvalues to compute.}

\item{which}{String. Selection criteria.}

\item{sym}{Logical scalar, whether the input matrix is symmetric.}

\item{options}{List. Additional options to the eigensolver.}

\item{env}{The environment in which \code{mul} will bevaluated.}

\item{values}{The eigenvalues}

\item{vectors}{The eigenvectors

The \code{options} argument specifies what kind of computation to perform.
It is a list with the following members, which correspond directly to
Anasazi parameters:

solver String. The name of the eigensolver to solve the eigenproblems.
            Currently, it supports three eigensolvers: KrylovSchur,
            Davidson and LOBPCG. KrylovSchur is the default eigensolver.

tol Numeric scalar. Stopping criterion: the relative accuracy of
            the Ritz value is considered acceptable if its error is less
            than \code{tol} times its estimated value.

block_size Numeric scalar. The eigensolvers use a block extension of an
            eigensolver algorithm. The block size determines the number
            of the vectors that operate together.

num_blocks Numeric scalar. The number of blocks to compute eigenpairs.}
}
\value{
\code{fm.eigen} returns a named list with the following members:
        values: Numeric vector, the desired eigenvalues.
        vectors: Numeric matrix, the desired eigenvectors as columns.
        \code{fm.cal.residul} returns the corresponding residuals for
        the eigenvalues.
}
\description{
\code{fm.eigen} computes eigenvalues/vectors of a square matrix.
\code{fm.cal.residul} computes the residual of the eigenvalues.
}
\details{
\code{fm.eigen} uses Anasazi package of Trilinos, if Anasazi is compiled
into FlashR, or eigs to compute eigenvalues.

The \code{which} specify which eigenvalues/vectors to compute, character
constant with exactly two characters. Possible values for symmetric input
matrices:
\itemize{
\item{"LA"}{Compute \code{nev} largest (algebraic) eigenvalues.}
\item{"SA"}{Compute \code{nev} smallest (algebraic) eigenvalues.}
\item{"LM"}{Compute \code{nev} largest (in magnitude) eigenvalues.}
\item{"SM"}{Compute \code{nev} smallest (in magnitude) eigenvalues.}
}
}
\examples{
mat <- fm.load.sparse.matrix("./spm123.mat", "./spm123.mat_idx")
res <- fm.eigen(mul, 10, nrow(mat))
fm.cal.residul(mul, res$values, res$vectors)
}
\author{
Da Zheng <dzheng5@jhu.edu>
}

