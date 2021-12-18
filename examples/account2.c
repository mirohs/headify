/*
make account2 && ./account2
*/

*#include "util.h"
#include "account2_headify.h"

*typedef struct {
    char* owner;
    int balance; }
Account;

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
