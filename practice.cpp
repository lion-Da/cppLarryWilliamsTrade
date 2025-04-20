#include <iostream> 
#include <cstdlib>
#include <ctime> 
using namespace std;

int main()
{
    srand(time(0));
    int randomNum = rand() % 100 + 1; 
    cout << "Random number between 1 and 100: " << randomNum << endl;
    return 0;
}