# WARNING
Do not upload any files in this repository to public websites. If you want to clone this repository, please make sure to keep it private.

# Starter code for CPSC457 Assignment 1
You can compile the C++ code by running:

```
$ make
```

You can test the slow C++ palindrome finder:

```
$ ./slow-pali < t4.txt
Longest palindrome: redder
```
To run the Python version:
```
$ ./palindrome.py < t4.txt
Longest palindrome: redder
```

# `dup.py` utility

Instead of storing really large files in your directory, and risking
running out of quota, you can use the included `dup.py` utility to generate large
input to your program.

`dup2.py` is a simple python program that accepts a single command
line argument `N`, which indicates the number of bytes that the script
will generate on standard output. `dup.py` reads in data from stdin,
byte by byte, and outputs the data to stdout. It always outputs N
bytes. If the data on stdin is bigger than N bytes, only first N bytes
are copied. If the data on stdin is shorter than N, then the script will
repeat the input data, until N bytes are generated. Example: 

```
$ echo "hello." | ./dup.py 10
hello.
hel
```

Here is an example of how to feed 2GB of data to your program, generated
by repeating `t3.txt`:
```
$ ./dup.py 2000000000 < t3.txt | ./fast-pali
```
Here is how you can time your code on the same data:
```
$ ./dup.py 2000000000 < t3.txt | time ./fast-pali
```
Here is how to run strace in combination with `dup.py`:
```
$ ./dup.py 2000000000 < t3.txt | strace -c ./fast-pali
```

