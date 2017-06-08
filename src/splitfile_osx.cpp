// http://www.matisse.net/bitcalc/?input_amount=150&input_units=megabits&notation=legacy

#include <iostream>
#include <fstream>

using namespace std;

// 150MB in KB
// http://www.matisse.net/bitcalc/?input_amount=150&input_units=megabits&notation=legacy
#define CHUNK2 157286400
#define CHUNK 134217728

void write_file(const string name, const char* buffer, const long size) {
    ofstream file("/home/atejeda/data0." + name, ios::out|ios::binary);
    if (file.is_open() && file.good()) {
        file.write(buffer, size);
        file.close();
    }
}

// rm -f splitfile && g++ splitfile_osx.cpp -o splitfile && ./splitfile
int main(int argc, char* argv[]) {
    
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

    int pieces = size / CHUNK;
    int remain = size % CHUNK;

    cout << "size " << size << " bytes" << endl;
    cout << pieces << " pieces of " << CHUNK << " bytes" << endl;
    cout << "a remaining of " << remain << " bytes" << endl;

    char* block_mem;
    long lower, upper, block_size;
    long sum = 0;

    for (int i = 0; i <= pieces; i++) {
        lower = i * CHUNK;
        upper = ((i + 1) * CHUNK) - 1;
        block_size = i < pieces ? CHUNK : remain;
        sum += block_size;

        block_mem = new char[block_size];
        iffile.read(block_mem, block_size);
        write_file(i < 10 ? '0' + to_string(i) : to_string(i), block_mem, block_size);
        delete block_mem;
        cout << i << " [" << lower << "," << upper << "], " << block_size << endl;
    }

    cout << "total sum = " << size - sum << endl;
    cout << "file position pointer is at the end? : " << ((iffile.tellg() == ios::end) ? "yes" : "no") << endl;
    iffile.close();

    return 0;
}