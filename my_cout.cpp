#include <cstdio>

class MyOutputStream
{


public:
    MyOutputStream &operator<<(const char *text)
    {
        std::printf("%s", text);
        return *this;
    }

    MyOutputStream &operator<<(int number)
    {
        std::printf("%d", number);
        return *this;
    }

    MyOutputStream &operator<<(double number)
    {
        std::printf("%f", number);
        return *this;
    }

    MyOutputStream &operator<<(char character)
    {
        std::printf("%c", character);
        return *this;
    }

    using Manipulator = MyOutputStream &(*)(MyOutputStream &);

    MyOutputStream &operator<<(Manipulator manip)
    {
        return manip(*this);
    }
};

MyOutputStream &my_endl(MyOutputStream &stream)
{
        std::printf("\n");
        return stream;
}

MyOutputStream my_cout;

int main()
{
        int year = 2025;
        my_cout << "Hello from the custom output stream!" << my_endl;
        my_cout << "The year is: " << year << my_endl;
        my_cout << "A double: " << 123.456 << my_endl;
        my_cout << 'A' << ' ' << 'c' << 'h' << 'a' << 'r' << my_endl;

    return 0;
}