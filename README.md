LLVM Challenge
==============
[![x86-Ubuntu](https://github.com/m7nu3l/llvm-tutor/actions/workflows/x86-ubuntu.yml/badge.svg?branch=main)](https://github.com/m7nu3l/llvm-tutor/actions/workflows/x86-ubuntu.yml)
[![docker-ubuntu](https://github.com/m7nu3l/llvm-tutor/actions/workflows/docker-ubuntu.yml/badge.svg?branch=main)](https://github.com/m7nu3l/llvm-tutor/actions/workflows/docker-ubuntu.yml)
 
### Interactive docker

* `git clone https://github.com/m7nu3l/llvm-tutor.git llvm-tutor`
* `cd llvm-tutor`
* `docker build -t llvm-challenge/llvm-challenge .`
* `docker run --rm -ti llvm-challenge/llvm-challenge`

This creates an interactive and temporal docker image where this project is already compiled and tested. Below, it is explained how to use the passes.

### How to build (Ubuntu 18.04)

#### Requirements

* `wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -`
* `sudo apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-12 main"`
* `sudo apt-get update`
* `sudo apt-get install -y gnuplot cmake llvm-12 llvm-12-dev clang-12 llvm-12-tools python3-setuptools`
* `sudo pip3 install lit`
* `sudo pip3 install py-gnuplot`

#### Building steps

* `git clone https://github.com/m7nu3l/llvm-tutor.git llvm-tutor`
* `cd llvm-tutor`
* `mkdir build`
* `cd build`
* `export LLVM_DIR=<installation/dir/of/llvm/12> # /usr/lib/llvm-12`
* `cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..`
* `make -j4`
* `rm -r -f test/ && lit -vv test/`

Histogram pass
==============

Given a function, the pass *appends* to a file **F** the number of instructions in each basic block. Then, that file can be processed by a python script (`repo/scripts/draw_histogram.py`) which draws the histogram. Under the hood, gnuplot is invoked.

#### opt

* `opt -load repo/build/lib/libHistogram.so -legacy-histogram -histogram-output-file=path/to/histogram.data`
* `python3 repo/scripts/draw_histogram.py path/to/histogram.data path/to/histogram.png`

#### clang

In this case, the pass is applied to every function processed by this clang call. All basic block sizes are appended to the same file.

* `clang-12 -O2 -fexperimental-new-pass-manager -fpass-plugin=/repo/build/lib/libHistogram.so` 
* `python3 repo/scripts/draw_histogram.py $(pwd)/histogram.data path/to/histogram.png`

Using the `-histogram-output` option, the output file path can be specified. However, this is only available for `opt`. The default path is `$(pwd)/histogram.data`. 

#### Logic

The pass entry point is [lib/Histogram.cpp::runOnFunction](https://github.com/m7nu3l/llvm-tutor/blob/main/lib/Histogram.cpp#L37).

#### LIT test case

Single-file program test is [test/Histogram-test.ll](https://github.com/m7nu3l/llvm-tutor/blob/main/test/Histogram-test.ll).

#### Open source project test

Source-code project test is [here](https://github.com/m7nu3l/llvm-tutor/blob/main/Dockerfile#L98).
The [zip project](https://github.com/kuba--/zip) is analyzed by the Histogram pass. Output files are in [/repo/histogram-sample](https://github.com/m7nu3l/llvm-tutor/tree/main/histogram-sample). 

Profiler pass
==============

Given a function, the pass *appends* a call to an external `bb_exec` function at the first available insertion point of each basic block. In addition, this project provides [a default implementation for it](https://github.com/m7nu3l/llvm-tutor/blob/main/lib/ProfilerRuntime.cpp#L44). This default implementation prints a message if a basic block is executed more than 1000 times. This threshold can be changed by the `PROFILER_THRESHOLD_ENV` environment variable.

A profiled binary only has external calls to `bb_exec`. In order to provide an actual implementation, `LD_PRELOAD` can be used. If no implementation is provided/linked, the binary can still be executed. The profiler injects a [trampoline function](https://github.com/m7nu3l/llvm-tutor/blob/main/lib/Profiler.cpp#L64) which checks that `bb_exec` is defined before calling it.

#### opt

* `opt -load  %shlibdir/libProfiler%shlibext -legacy-profiler`

#### clang

In this case, the pass is applied to every function processed by this clang call. 

* `clang-12 -O2 -fexperimental-new-pass-manager -fpass-plugin=/llvm-tutor/build/lib/libProfiler.so` 

#### Logic

The pass entry point is [lib/Profiler.cpp::runOnFunction](https://github.com/m7nu3l/llvm-tutor/blob/main/lib/Profiler.cpp#L117).

#### LIT test case

Single-file program test is [test/Profiler-test-1.c](https://github.com/m7nu3l/llvm-tutor/blob/main/test/Profiler-test-1.c).

#### Open source project test

Source-code project test is [here](https://github.com/m7nu3l/llvm-tutor/blob/main/Dockerfile#L88).
The [zip project](https://github.com/kuba--/zip) is transformed by the Profiler pass. An example of the output can be found [here](https://github.com/m7nu3l/llvm-tutor/runs/3099304204#step:3:2832)

```
5: [PROFILER] Basic block '%150' from function 'tdefl_compress_normal' in module '/zip-profiled/src/zip.c' was executed more than 1000 times.
5: [PROFILER] Basic block '%32' from function 'tdefl_compress_normal' in module '/zip-profiled/src/zip.c' was executed more than 1000 times.
5: [PROFILER] Basic block '%45' from function 'tdefl_compress_normal' in module '/zip-profiled/src/zip.c' was executed more than 1000 times.
```

The zip project test-suite [is successful](https://github.com/m7nu3l/llvm-tutor/runs/3099304204#step:3:2858) after being profiled: `6 tests, 93 assertions, 0 failures`

Limitations/Future work
=======================

* `ProfileRuntime` is not meant to process multi-threaded code.
* Improve histogram drawing.
* Histogram pass's output file should have extra columns to identify each basic block's size (function & module).
* Profiler pass is duplicating constants strings rather than re-using them.
* Is `bb_exec` GV correctly initialized to `nullptr` if it is not linked? So far, it looks like yes.  
* Improve at which point of the optimisation pipeline Histogram and Profiler passes are registered in clang.
