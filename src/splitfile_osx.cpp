// http://www.matisse.net/bitcalc/?input_amount=150&input_units=megabits&notation=legacy

#include <iostream>
#include <fstream>

using namespace std;

// https://wiki.ubuntu.com/UnitsPolicy
#define KILO 1000
#define MEGS 150
#define MB150 (1L * KILO * KILO * MEGS)

void write_file(const string name, const char* buffer, const long size) {
    ofstream file("/home/atejeda/data0." + name, ios::out|ios::binary);
    if (file.is_open() && file.good()) {
        file.write(buffer, size);
        file.close();
    }
}

// rm -f splitfile && g++ splitfile_osx.cpp -o splitfile && ./splitfile
int main(int argc, char* argv[]) {
    const long xMB = MB150;
    
    // dd if=/dev/zero of=~/1GB.dat bs=100M count=10
    // dd if=/dev/zero of=~/1GB.dat bs=262144000 count=4

    ifstream iffile;
    iffile.open("/home/atejeda/data0", ios::in|ios::binary|ios::ate);
    
    if (!iffile.is_open() || !iffile.good()) {
        cout << "there's some error opening file..." << endl;
        return 0;
    }

    long size = iffile.tellg();
    iffile.seekg(0, ios::beg);

    int pieces = size / xMB;
    int remain = size % xMB;

    cout << "size " << size << " bytes" << endl;
    cout << pieces << " pieces of " << xMB << " bytes" << endl;
    cout << "a remaining of " << remain << " bytes" << endl;

    char* block_mem;
    long lower, upper, block_size, sum = 0;

    for (int i = 0; i <= pieces; i++) {
        lower = i * xMB;
        upper = ((i + 1) * xMB) - 1;
        block_size = i < pieces ? xMB : remain;
        sum += block_size;

        block_mem = new char[block_size];
        iffile.read(block_mem, block_size);
        write_file(i < 10 ? '0' + to_string(i) : to_string(i), block_mem, block_size);
        delete block_mem;
        cout << i << " [" << lower << "," << upper << "], " << block_size << endl;
    }

    cout << "total sum = " << size - sum << endl;
    iffile.close();

    return 0;
}