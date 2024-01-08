#ifndef MIPSHELPER_H
#define MIPSHELPER_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <deque>
#include <algorithm>
using namespace std;

void add(int d, int s, int t);
void sub(int d, int s, int t);
void mult(int s, int t);
void divide(int s, int t);
void mfhi(int d);
void mflo(int d);
void lis(int d);
void slt(int d, int s, int t);
void sltu(int d, int s, int t);

void jr(int s);
void jalr(int s);
void beq(int s, int t, string label);
void bne(int s, int t, string label);
void lw(int t, int i, int s);
void sw(int t, int i, int s);

void word(int i);
void word(string label);
void label(string name);

void push(int s);
void pop(int d);
void pop();

#endif