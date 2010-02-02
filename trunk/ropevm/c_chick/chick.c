#include <stdio.h>

/*
  class Chick
*/
typedef struct {
    int x;
} Chick;

/*
  void Chick::f()
*/
void Chick_f(Chick* this)
{
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
        this->x++;
}

/*
  void Chick::getResult()
*/
int Chick_getResult(Chick* this)
{
    return this->x;
}

/*-----------------------------------------*/

Chick a;
Chick b;
Chick c;
Chick d;
Chick e;
Chick f;

int test1(void)
{
    int n;
    for (n = 0; n < 10; ++n) {
        Chick_f(&a);
        Chick_f(&b);
        Chick_f(&c);
        Chick_f(&d);
        Chick_f(&e);
        Chick_f(&f);
    }

    int result = 0;
    result += Chick_getResult(&a);
    result += Chick_getResult(&b);
    result += Chick_getResult(&c);
    result += Chick_getResult(&d);
    result += Chick_getResult(&e);
    result += Chick_getResult(&f);

    return result;
}

Chick all[6];
int test2(void)
{
    int n, i;
    for (n = 0; n < 10; ++n) {
        for (i = 0; i < 6; ++i)
            Chick_f(&all[i]);
    }

    int result = 0;
    for (i = 0; i < 6; ++i)
        result += Chick_getResult(&all[i]);

    return result;
}

int main(void)
{
    int result= 0;
    result = test1();
    //result = test2();
    printf("result is: %d\n", result);

    return 0;
}

