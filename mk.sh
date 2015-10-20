#!/bin/bash
gcc -O0 -g main.c walker.c threadpool.c fdopt.c conf.c http.c -o walker  -lpthread
