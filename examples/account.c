*#include "util.h"

;
int ibalance(int a) {
    return a;
 }

*int ff(int a, int b);
*char* hello = "hello";// 	hello 
*int i;

*typedef void (*MyFuncType)(void);

/*
make account && ./account
*/;

/*abc*//*def*/#include "util.h"

*# /*mytest*/ define bo (
*# define bc ) // test

*typedef struct {
    char* owner;
    int balance; }
Account;

*Account acc = {NULL, 123};

*struct Account;

*struct Point {
    int x;
    int y; };

int a[10]; // line comment
*int apub[10]; // line comment
*int b[2][3] = {{1, 2, 3}, {4, 5, 6}};
double d = 3.14; 
    #define xyz 123

*typedef int (*fp)(double, double);

Account open_account(char* owner, int initial) {
    require("not empty", strlen(owner) > 0);
    require("not negative", initial >= 0);
    Account a;
    a.owner = owner;
    a.balance = initial;
    return a; }



Account deposit(Account a, int amount) {
    require("not n\
    egative", amount >= 0);
    ensure_code(int old_balance = a.balance);
    a.balance += amount;
    ensure("amount added", a.balance == old_balance + amount);
    return a; }




Account withdraw(Account a, int amount) {
    require("not negative", amount >= 0);
    require("sufficient balance", a.balance >= amount);
    ensure_code(int old_balance = a.balance);
    a.balance -= amount;
    ensure("amount subtracted", a.balance == old_balance - amount);
    ensure("balance not negative", a.balance >= 0);
    return a; }


*int balance(Account a) {
    return a.balance; }


char* owner(Account a) {
    return a.owner; }

*union IntOrFLoat {
    int i;
    float f; };

union IntOrFloat { int i; float f; };

 *int main(void) {
    Account a = open_account("Ida", 100);
    printf("%d\n", balance(a));
    a = deposit(a, 10);
    printf("%d\n", balance(a));
    a = withdraw(a, 20);
    
    printf("%d\n", balance(a));
    a = withdraw(a, 80);
    printf("%d\n", balance(a));
    return 0; }
