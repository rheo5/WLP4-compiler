#include "mipshelper.h"

void add(int d, int s, int t){
    cout << "add $" << d << ", $" << s << ", $" << t << "\n"; 
}
void sub(int d, int s, int t){
    cout << "sub $" << d << ", $" << s << ", $" << t << "\n"; 
}
void mult(int s, int t){
    cout << "mult $" << s << ", $" << t << "\n"; 
}
void divide(int s, int t){
    cout << "div $" << s << ", $" << t << "\n"; 
}
void mfhi(int d){
    cout << "mfhi $" << d << "\n";
}
void mflo(int d){
    cout << "mflo $" << d << "\n";
}
void lis(int d){
    cout << "lis $" << d << "\n";
}
void slt(int d, int s, int t){
    cout<< "slt $" << d << ", $" << s << ", $" << t << "\n";
}

void sltu(int d, int s, int t){
    cout<< "sltu $" << d << ", $" << s << ", $" << t << "\n";
}

void jr(int s){
    cout << "jr $"  << s << "\n"; 
}
void jalr(int s){
    cout << "jalr $"  << s << "\n"; 
}

void beq(int s, int t, string label){
    cout << "beq $" << s << ", $" << t << ", " + label + "\n"; 
}
void bne(int s, int t, string label){
    cout << "bne $" << s << ", $" << t << ", " + label + "\n"; 
}

void lw(int t, int i, int s) {
    cout << "lw $" << t << ", " << i << "($" << s << ")\n";
}
void sw(int t, int i, int s) {
    cout << "sw $" << t << ", " << i << "($" << s << ")\n";
}

void word(int i){
    cout << ".word " << i << "\n";
}
void word(string label){
    cout << ".word " + label + "\n";
}
void label(string name){
    cout << name + ":\n";
}

void push(int s){
    cout<< "sw $"<< s <<", -4($30)\n";
    cout<< "sub $30, $30, $4\n";
}

void pop(int d){
    cout<< "add $30, $30, $4\n";
    cout<< "lw $"<< d << ", -4($30)\n";
}
void pop(){
    cout<< "add $30, $30, $4\n";
}