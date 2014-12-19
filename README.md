SVDLIBC-BLAS
============

This is a blas-vesion of SVDLIBC originally from

  http://tedlab.mit.edu/~dr/SVDLIBC/

Please modify the `LIBS` line of `Makefile` to link a
specific BLAS implementation of your preference such as
OpenBLAS, ATLAS, or MKL. Or you can update the default BLAS
library by

```bash:Debian/Ubuntu
$ sudo update-alternatives --config libblas.so
```
